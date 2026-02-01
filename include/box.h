//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_BOX_H
#define SODADOWNLOADER_BOX_H
#include <cstdint>
#include <string>

struct box
{
    uint32_t size;
    std::string type;
    size_t offset;

    box(uint32_t s, const std::string& t, size_t o) : size(s), type(t), offset(o)
    {
    }
};

#endif //SODADOWNLOADER_BOX_H
