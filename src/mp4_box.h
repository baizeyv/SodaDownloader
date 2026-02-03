//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_MP4_BOX_H
#define SODADOWNLOADER_MP4_BOX_H
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

struct mp4_box
{
    uint32_t size;
    
    string type;
    
    size_t offset;
    
    vector<uint8_t> data;
    
    mp4_box(uint32_t _size, const string& _type, size_t _offset, const vector<uint8_t>& _data);
    
    mp4_box(const vector<uint8_t>& file_data, size_t off);
    
    bool empty() const;
    
    static mp4_box find_box(const vector<uint8_t>& file_data, const string& box_type, size_t offset = 0, size_t end = SIZE_MAX);
    
};


#endif //SODADOWNLOADER_MP4_BOX_H