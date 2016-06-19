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

void DataGenerator::FillBuffer(quint64 full_index, quint64 ms_time_offset, quint8 *buffer)
{
    LEDScreenState state = {0};

    quint16 hash = crc.XModemCrc((uint8_t*) (void*) &state, 0, sizeof(state)-sizeof(quint16));
    std::memcpy(buffer, &state, sizeof state);
}
