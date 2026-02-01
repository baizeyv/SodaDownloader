//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_MP4_BOX_H
#define SODADOWNLOADER_MP4_BOX_H
#include <cstdint>
#include <vector>
#include <string>

struct MP4Box
{
    uint32_t size;
    std::string type;
    size_t offset;
    std::vector<uint8_t> data;
    
    MP4Box(uint32_t _size, std::string _type, size_t _offset, std::vector<uint8_t> _data);
    
    MP4Box(const std::vector<uint8_t>& fileData, size_t off);
    
    bool empty() const;
    
    static MP4Box findBox(const std::vector<uint8_t>& fileData, const std::string& boxType, size_t offset = 0, size_t end = SIZE_MAX);
};

#endif //SODADOWNLOADER_MP4_BOX_H