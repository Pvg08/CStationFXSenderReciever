#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include "Crc16.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 4

#if defined(WIN32) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
    #define __PACKED                         /* dummy */
#else
    #define __PACKED __attribute__((packed)) /* gcc packed */
#endif

#if defined(WIN32) || defined(_WIN64) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
#pragma pack(push, 1)
#endif

typedef uint8_t LEDMatrixState[MATRIX_ROWS_COUNT] __PACKED;
struct LEDScreenState {
    uint32_t block_index __PACKED;
    uint32_t timeout __PACKED;
    LEDMatrixState blocks[MATRIX_COUNT] __PACKED;
    uint8_t played __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

#if defined(WIN32) || defined(_WIN64) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
#pragma pack(pop)
#endif

class DataGenerator
{
public:
    DataGenerator();
    quint16 getDataSize();
    quint16 getBufferSize();

    LEDScreenState getNextState(uint32_t full_index);
    LEDScreenState getEmptyState(uint32_t full_index);
private:
    quint16 state_size;
    quint16 state_count;
    Crc16 crc;
};

#endif // DATAGENERATOR_H
