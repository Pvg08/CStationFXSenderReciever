#include "serialfxwriter.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

QT_USE_NAMESPACE

SerialFXWriter::SerialFXWriter(QObject *parent)
    : QThread(parent), waitTimeout(0), quit(false)
{
}

SerialFXWriter::~SerialFXWriter()
{
    mutex.lock();
    quit = true;
    mutex.unlock();
    wait();
}

void SerialFXWriter::listen(const QString &portName, int waitTimeout, DataGenerator* c_generator)
{
    QMutexLocker locker(&mutex);
    this->portName = portName;
    this->waitTimeout = waitTimeout;
    generator = c_generator;
    half_buf_size = generator->getBufferSize() / 2;
    resetBuffers();
    if (!isRunning())
        start();
}

void SerialFXWriter::run()
{
    bool currentPortNameChanged = false;
    bool data_written = false;

    mutex.lock();
    QString currentPortName;
    if (currentPortName != portName) {
        currentPortName = portName;
        currentPortNameChanged = true;
    }
    int currentWaitTimeout = waitTimeout;
    mutex.unlock();

    QSerialPort serial;

    serial.setBaudRate(115200);

    while (!quit) {

        if (currentPortNameChanged) {
            serial.close();
            serial.setPortName(currentPortName);

            if (!serial.open(QIODevice::ReadWrite)) {
                emit error(tr("Can't open %1, error code %2")
                           .arg(portName).arg(serial.error()));
                return;
            }
        }

        fillBuffer();

        while(request_write_position->block_index < send_buffer.end()->block_index) {
            request_write_position++;
            serial.write((char*)(void*) &(request_write_position.i->t), sizeof(LEDScreenState));
            data_written = true;
            emit log(tr("Writing state %1 to serial...").arg(request_write_position->block_index));
        }

        if (data_written) {
            if (!serial.waitForBytesWritten(waitTimeout)) {
                emit timeout(tr("Wait write request timeout %1")
                             .arg(QTime::currentTime().toString()));
            }
            data_written = false;
        }

        // read response
        if (serial.waitForReadyRead(currentWaitTimeout)) {
            emit log("Ready read...");
            QByteArray responseData = serial.readAll();
            while (serial.waitForReadyRead(10))
                responseData += serial.readAll();

            responseCheck(responseData);
        } else {
            emit timeout(tr("Wait read response timeout %1")
                         .arg(QTime::currentTime().toString()));
        }

        mutex.lock();
        if (currentPortName != portName) {
            currentPortName = portName;
            currentPortNameChanged = true;
        } else {
            currentPortNameChanged = false;
        }
        currentWaitTimeout = waitTimeout;
        mutex.unlock();
    }
}

void SerialFXWriter::do_stop()
{
    if (isRunning()) quit = true;
}

void SerialFXWriter::resetBuffers()
{
    emit log("Resetting buffers...");
    LEDScreenState state = {0};
    full_index = 0;
    send_buffer.clear();
    state = generator->getNextState(full_index++);
    send_buffer.append(state);
    request_write_position = send_buffer.begin();
    request_confirm_position = send_buffer.begin();
}

void SerialFXWriter::fillBuffer()
{
    LEDScreenState state = {0};
    while(send_buffer.size() < half_buf_size) {
        emit log("Generating block "+QString::number(full_index)+"...");
        state = generator->getNextState(full_index++);
        send_buffer.append(state);
    }
}

void SerialFXWriter::responseCheck(QByteArray response)
{
    emit this->response(((QString) response.toHex()) + " (" + QString::fromLocal8Bit(response) + ")");

    bool confirm_set = false;
    uint32_t last_confirmed_position;
    uint8_t data[4];
    for(int i=0; i<response.size()-6; i++) {
        if (response.at(i) == 'S' && response.at(i+1) == 'C' && response.at(i+2) == 'E') {
            data[0] = response.at(i+3);
            data[1] = response.at(i+4);
            data[2] = response.at(i+5);
            data[3] = response.at(i+6);
            last_confirmed_position = *((quint32*)(void*)data);
            confirm_set = true;
            i+=6;
        }
    }
    if (confirm_set) setConfirmPosition(last_confirmed_position);
}

void SerialFXWriter::setConfirmPosition(uint32_t confirm_position)
{
    emit log("Confirmed position: "+QString::number(confirm_position));
    request_confirm_position = send_buffer.begin();
    while((request_confirm_position!=send_buffer.end()) && (request_confirm_position->block_index < confirm_position)) {
        request_confirm_position++;
        send_buffer.removeFirst();
    }
    request_write_position = request_confirm_position;
}
