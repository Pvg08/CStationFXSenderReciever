#include "datagenerator.h"

DataGenerator::DataGenerator()
{
    state_size = sizeof(LEDScreenState);
    state_count = MATRIX_STATE_BUFFER_SIZE;
}

uint16_t DataGenerator::getDataSize()
{
    return state_size;
}

uint16_t DataGenerator::getBufferSize()
{
    return state_count;
}

void DataGenerator::fillNextState(uint32_t full_index, LEDScreenState* state)
{
    state->state_index = full_index;

    unsigned col = full_index % 8;
    full_index /= 8;
    unsigned row = full_index % 8;
    full_index /= 8;
    unsigned page = full_index % 5;

    state->blocks[page][row] = 1 << col;
    state->timeout = 50;
    state->played = 0;
    state->hash = 0;
    state->hash = crc.XModemCrc((uint8_t*) (void*) state, 0, sizeof(state));
}

void DataGenerator::fillEmptyState(uint32_t full_index, LEDScreenState* state)
{
    state->state_index = full_index;
    state->played = 0;
    state->hash = 0;
    state->hash = crc.XModemCrc((uint8_t*) (void*) &state, 0, sizeof(state));
}
