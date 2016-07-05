#ifndef DATAGENERATOR_H
#define DATAGENERATOR_H

#include <QObject>
#include <math.h>
#include "Crc16.h"

#define MATRIX_COUNT 5
#define MATRIX_ROWS_COUNT 8
#define MATRIX_STATE_BUFFER_SIZE 8

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
    uint32_t state_index __PACKED;
    uint32_t timeout __PACKED;
    LEDMatrixState blocks[MATRIX_COUNT] __PACKED;
    uint16_t played __PACKED;
    uint16_t hash __PACKED;
} __PACKED;

#if defined(WIN32) || defined(_WIN64) || defined(__WATCOMC__) || defined(_WIN32) || defined(__WIN32__)
#pragma pack(pop)
#endif

class DataGenerator
{
public:
    DataGenerator();
    uint16_t getDataSize();
    uint16_t getBufferSize();

    void fillNextState(uint32_t full_index, LEDScreenState* state);
    void fillEmptyState(uint32_t full_index, LEDScreenState* state);
private:
    uint16_t state_size;
    uint16_t state_count;
    Crc16 crc;
};

#endif // DATAGENERATOR_H
