#include "serialfxwriter.h"

#include <QtSerialPort/QSerialPort>
#include <QTime>

QT_USE_NAMESPACE

SerialFXWriter::SerialFXWriter(QObject *parent)
    : QThread(parent), waitTimeout(0), quit(false)
{
    state_index = 0;
    request_write_position = 0;
    request_confirm_position = 0;
    request_generate_position = 0;
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

        if (request_write_position != request_generate_position) {
            while(request_write_position != request_generate_position) {
                request_write_position = getNextPosition(request_write_position);
                LEDScreenState state = send_buffer.at(request_write_position);
                serial.write((char*)(void*) &state, sizeof(LEDScreenState));
                emit log(tr("Writing state %1 to serial...").arg(state.state_index));
                data_written = true;
            }
            request_write_position = request_generate_position;
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
    emit log(QString("Resetting buffers (size = %1)...").arg(sizeof(LEDScreenState)));
    state_index = 0;
    send_buffer.clear();
    LEDScreenState state = {0};
    generator->fillEmptyState(state_index, &state);
    while (send_buffer.size() < generator->getBufferSize()) {
        send_buffer.append(state);
    }
    request_write_position = request_confirm_position = request_generate_position = send_buffer.size()-1;
}

void SerialFXWriter::fillBuffer()
{
    unsigned interval = 0;

    if (request_confirm_position < request_generate_position) {
        interval = request_generate_position - request_confirm_position;
    } else if (request_confirm_position == request_generate_position) {
        if (request_write_position != request_generate_position) {
            interval = send_buffer.size();
        } else {
            interval = 0;
        }
    } else {
        interval = request_confirm_position + send_buffer.size() - request_generate_position;
    }

    emit log("Buffer interval: "+QString::number(interval));

    if (interval<half_buf_size) {
        for(interval=0; interval<half_buf_size; interval++) {
            request_generate_position = getNextPosition(request_generate_position);
            LEDScreenState state = {0};
            generator->fillNextState(state_index++, &state);
            send_buffer[request_generate_position] = state;
            emit log("Generating block "+QString::number(state.state_index)+"...");
        }
    }
    emit log("Buffer genpos: "+QString::number(request_generate_position));
}

void SerialFXWriter::responseCheck(QByteArray response)
{
    emit this->response(((QString) response.toHex()) + " (" + QString::fromLocal8Bit(response) + ")");

    bool confirm_set = false;
    uint32_t last_confirmed_position;
    uint8_t data[4];
    for(int i=0; i<response.size()-6; i++) {
        if (response.at(i) == 'C' && response.at(i+1) == 'S' && response.at(i+2) == 'P') {
            data[0] = response.at(i+3);
            data[1] = response.at(i+4);
            data[2] = response.at(i+5);
            data[3] = response.at(i+6);
            last_confirmed_position = *((uint32_t*)(void*)data);
            confirm_set = true;
            i+=6;
        }
    }
    if (confirm_set) setConfirmPosition(last_confirmed_position);
}

void SerialFXWriter::setConfirmPosition(uint32_t confirm_position)
{
    emit log("Confirmed position: "+QString::number(confirm_position));

    unsigned i;
    for(i=0; i<send_buffer.size() && send_buffer.at(i).state_index != confirm_position; i++) {
        emit log("CPOS["+QString::number(i)+"]: "+QString::number(send_buffer.at(i).state_index) + " <> " + QString::number(confirm_position));
    }
    if (i<send_buffer.size()) {
        request_confirm_position = i;
        request_write_position = i;
        emit log("Confirmed request_confirm_position="+QString::number(request_confirm_position) +", request_write_position="+QString::number(request_write_position));
    } else {
        emit log("Confirmed request_confirm_position not found");
    }
}

unsigned SerialFXWriter::getNextPosition(unsigned cur_position)
{
    cur_position++;
    if (cur_position>=send_buffer.size()) cur_position = 0;
    return cur_position;
}
