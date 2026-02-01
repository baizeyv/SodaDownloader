//
// Created by baizeyv on 2/1/2026.
//

#include "../include/mp4_box.h"

#include <iostream>
#include <stdexcept>

#include "../include/utils.h"

MP4Box::MP4Box(uint32_t _size, std::string _type, size_t _offset, std::vector<uint8_t> _data)
{
    size = _size;
    type = _type;
    offset = _offset;
    data = _data;
}

MP4Box::MP4Box(const std::vector<uint8_t>& fileData, size_t off) : offset(off)
{
    if (offset + 8 > fileData.size()) throw std::runtime_error("MP4Box 构造失败: 数据太短");

    size = utils::readUint32BE(fileData, offset);
    type = utils::readBoxType(fileData, offset);

    if (offset + size > fileData.size()) size = fileData.size() - offset; // 防越界
    data = std::vector<uint8_t>(fileData.begin() + offset + 8, fileData.begin() + offset + size);
}

bool MP4Box::empty() const
{
    if (size == 999 && offset == 999 && type == "999")
        return true;
    return false;
}

MP4Box MP4Box::findBox(const std::vector<uint8_t>& fileData, const std::string& boxType, size_t offset, size_t end)
{
    if (end == SIZE_MAX) end = fileData.size();
    size_t pos = offset;

    while (pos < end)
    {
        if (pos + 8 > end) break;

        uint32_t size = utils::readUint32BE(fileData, pos);
        if (size < 8 || size > end - pos) break;

        std::string currentType = utils::readBoxType(fileData, pos);
        if (currentType == boxType)
        {
            // 找到目标 Box，返回指针（注意内存管理）
            return MP4Box(fileData, pos);
        }
        pos += size;
    }
    return {999, "999", 999, {}};
}
