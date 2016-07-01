#include "datagenerator.h"

DataGenerator::DataGenerator()
{
    state_size = sizeof(LEDScreenState);
    state_count = MATRIX_STATE_BUFFER_SIZE;
}

quint16 DataGenerator::getDataSize()
{
    return state_size;
}

quint16 DataGenerator::getBufferSize()
{
    return state_count;
}

LEDScreenState DataGenerator::getNextState(uint32_t full_index)
{
    LEDScreenState state = {0};

    unsigned col = full_index % 8;
    full_index /= 8;
    unsigned row = full_index % 8;
    full_index /= 8;
    unsigned page = full_index % 5;

    state.block_index = full_index;
    state.blocks[page][row] = 1 << col;
    state.timeout = 1000;

    state.played = 0;
    state.hash = 0;
    state.hash = crc.XModemCrc((uint8_t*) (void*) &state, 0, sizeof(state));
    return state;
}

LEDScreenState DataGenerator::getEmptyState(uint32_t full_index)
{
    LEDScreenState state = {0};
    state.block_index = full_index;
    state.played = 0;
    state.hash = 0;
    state.hash = crc.XModemCrc((uint8_t*) (void*) &state, 0, sizeof(state));
    return state;
}
