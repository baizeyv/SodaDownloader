//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_DECRYPT_UTILS_H
#define SODADOWNLOADER_DECRYPT_UTILS_H
#include <cstdint>
#include <string>
#include <vector>

#include "stsc_entry.h"

using namespace std;


class decrypt_utils
{
public:
    
    static int bit_count(int n);
    
    static uint8_t decode_base_36(uint8_t c);
    
    static vector<uint8_t> decrypt_spade_inner(const vector<uint8_t>& spade_key);
    
    static string decrypt_spade(const vector<uint8_t>& spade_key_bytes);
    
    static string decrypt_spade_a(const string& spade_a);
    
    static vector<uint8_t> aes_ctr_decrypt(const vector<uint8_t>& key, const vector<uint8_t>& iv, const vector<uint8_t>& data);
    
    static vector<uint32_t> parse_stsz(const vector<uint8_t>& data);
    
    static vector<stsc_entry> parse_stsc(const vector<uint8_t>& data);
    
    static vector<vector<uint8_t>> parse_senc(const vector<uint8_t>& data);
    
    static vector<uint8_t> update_stco(const vector<uint8_t>& data, const vector<uint32_t>& offsets);
    
    static vector<uint8_t> scan_for_flac_metadata(const vector<uint8_t>& stsd_data);
    
    static vector<uint32_t> calculate_chunk_offsets(const vector<uint32_t>& sample_sizes, const vector<stsc_entry>& stsc, uint32_t chunk_count, uint32_t base_offset);
    
    static uint32_t read_uint_32_big_end(const vector<uint8_t>& data, size_t offset);
    
    static vector<uint8_t> write_uint_32_big_end(uint32_t value);
    
    static void write_uint_32_big_end(uint8_t* buf, uint32_t value);
    
    static vector<uint8_t> hex_to_bytes(const string& hex);
    
    static string read_box_type(const vector<uint8_t>& data, size_t offset);
    
    static vector<uint8_t> concat_uint_8_arrays(const vector<vector<uint8_t>>& arrays);
    
};


#endif //SODADOWNLOADER_DECRYPT_UTILS_H