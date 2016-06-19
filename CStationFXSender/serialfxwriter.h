#ifndef SERIALFXWRITER_H
#define SERIALFXWRITER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "datagenerator.h"

#define BUFFER_COUNT 20

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

private:
    QString portName;
    QVector<QByteArray> request;
    unsigned request_write_position, request_generate_position, request_confirm_position;
    int waitTimeout;
    QMutex mutex;
    QWaitCondition cond;
    bool quit;
    DataGenerator* generator;

    void nextMatrixState();
    void resetBuffers();
    void writeNext();
};

#endif // SERIALFXWRITER_H
