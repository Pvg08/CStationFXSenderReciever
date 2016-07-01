#ifndef SERIALFXWRITER_H
#define SERIALFXWRITER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QLinkedList>

#include "datagenerator.h"

class SerialFXWriter : public QThread
{
    Q_OBJECT

public:
    SerialFXWriter(QObject *parent = 0);
    ~SerialFXWriter();

    void listen(const QString &portName, int waitTimeout, DataGenerator* c_generator);
    void run();
    void do_stop();

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);
    void log(const QString &s);

private:
    QString portName;
    QLinkedList<LEDScreenState> send_buffer;
    QLinkedList<LEDScreenState>::iterator request_write_position, request_confirm_position;
    int waitTimeout;
    QMutex mutex;
    bool quit;
    DataGenerator* generator;
    quint64 full_index;
    unsigned half_buf_size;

    void resetBuffers();
    void fillBuffer();
    void responseCheck(QByteArray response);

    void setConfirmPosition(uint32_t confirm_position);
};

#endif // SERIALFXWRITER_H
