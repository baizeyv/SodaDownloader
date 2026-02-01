//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_STSCENTRY_H
#define SODADOWNLOADER_STSCENTRY_H
#include <cstdint>

struct StscEntry
{
    uint32_t firstChunk;
    uint32_t samplesPerChunk;
    uint32_t id; // sample description index
};
#endif //SODADOWNLOADER_STSCENTRY_H
