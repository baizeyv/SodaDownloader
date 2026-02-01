//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_MP4_BOX_H
#define SODADOWNLOADER_MP4_BOX_H
#include <vector>

struct mp4_box
{
    size_t offset;
    size_t size;
    std::vector<uint8_t> data;
};

#endif //SODADOWNLOADER_MP4_BOX_H