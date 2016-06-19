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
    cond.wakeOne();
    mutex.unlock();
    wait();
}

void SerialFXWriter::listen(const QString &portName, int waitTimeout, DataGenerator* c_generator)
{
    QMutexLocker locker(&mutex);
    this->portName = portName;
    this->waitTimeout = waitTimeout;
    this->generator = c_generator;
    this->request_write_position = 0;
    this->request_generate_position = 0;
    this->request_confirm_position = 0;
    resetBuffers();
    if (!isRunning())
        start();
    else
        cond.wakeOne();
}

void SerialFXWriter::run()
{
    bool currentPortNameChanged = false;

    mutex.lock();
    QString currentPortName;
    if (currentPortName != portName) {
        currentPortName = portName;
        currentPortNameChanged = true;
    }

    int currentWaitTimeout = waitTimeout;
    QByteArray currentRequest = request;
    mutex.unlock();

    QSerialPort serial;

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

        // write request
        serial.write(currentRequest);
        if (serial.waitForBytesWritten(waitTimeout)) {
            // read response
            if (serial.waitForReadyRead(currentWaitTimeout)) {
                QByteArray responseData = serial.readAll();
                while (serial.waitForReadyRead(10))
                    responseData += serial.readAll();

                QString response(responseData);

                emit this->response(response);

            } else {
                emit timeout(tr("Wait read response timeout %1")
                             .arg(QTime::currentTime().toString()));
            }
        } else {
            emit timeout(tr("Wait write request timeout %1")
                         .arg(QTime::currentTime().toString()));
        }
        mutex.lock();
        cond.wait(&mutex);
        if (currentPortName != portName) {
            currentPortName = portName;
            currentPortNameChanged = true;
        } else {
            currentPortNameChanged = false;
        }
        currentWaitTimeout = waitTimeout;


        nextMatrixState();

        currentRequest = request;
        mutex.unlock();
    }
}

void SerialFXWriter::do_stop()
{
    if (isRunning()) quit = true;
}

void SerialFXWriter::nextMatrixState()
{
    quint8 arr_len = MATRIX_COUNT*8;
    char arr[MATRIX_COUNT*8];

    for(quint8 i=0; i<arr_len; i++) {
        arr[i] = (qrand() % 255) & (qrand() % 255);
    }

    request.setRawData(arr, MATRIX_COUNT*8);
}

void SerialFXWriter::resetBuffers()
{
    // @todo
}

void SerialFXWriter::writeNext()
{
    // @todo
    // generator->FillBuffer(...);
}
