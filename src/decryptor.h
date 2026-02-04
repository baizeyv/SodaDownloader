//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_DECRYPTOR_H
#define SODADOWNLOADER_DECRYPTOR_H
#include <vector>
#include <string>

#include "media_info.h"
#include "stsc_entry.h"

using namespace std;


class decryptor
{
    vector<uint32_t> g_sample_sizes {};
    
    vector<stsc_entry> g_stsc_entries {};
    
    uint32_t g_chunk_count = 0;
    
    vector<uint8_t> process_box_tree(const vector<uint8_t>& data, size_t offset, size_t size, uint32_t new_mdat_offset);
    
public:
    
    void decrypt(const model_info& model, const media_info& media, const string& output_path);
};


#endif //SODADOWNLOADER_DECRYPTOR_H