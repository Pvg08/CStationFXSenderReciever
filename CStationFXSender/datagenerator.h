#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include "Crc16.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 100

typedef quint8 LEDMatrixState[MATRIX_ROWS_COUNT];
struct LEDScreenState {
    quint16 block_index;
    quint16 timeout;
    LEDMatrixState blocks[MATRIX_COUNT];
    quint16 hash;
};

class DataGenerator
{
public:
    DataGenerator();
    quint16 getDataSize();
    quint16 getBufferSize();

    void FillBuffer(quint64 full_index, quint64 ms_time_offset, quint8* buffer);
private:
    quint16 state_size;
    quint16 state_count;
    Crc16 crc;
};

#endif // DATAGENERATOR_H
