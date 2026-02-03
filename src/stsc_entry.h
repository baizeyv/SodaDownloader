//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_STSC_ENTRY_H
#define SODADOWNLOADER_STSC_ENTRY_H
#include <cstdint>


struct stsc_entry
{
    
    uint32_t first_chunk;
    
    uint32_t samples_per_chunk;
    
    uint32_t id; // # sample description index
    
};


#endif //SODADOWNLOADER_STSC_ENTRY_H