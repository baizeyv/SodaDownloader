//
// Created by baizeyv on 2/3/2026.
//

#include "mp4_box.h"

#include <stdexcept>

#include "decrypt_utils.h"

mp4_box::mp4_box(const uint32_t _size, const string& _type, const size_t _offset, const vector<uint8_t>& _data)
{
    size = _size;
    type = _type;
    offset = _offset;
    data = _data;
}

mp4_box::mp4_box(const vector<uint8_t>& file_data, size_t off) : offset(off)
{
    if (offset + 8 > file_data.size())
        throw new runtime_error("build mp4_box failed: data is too short.");
    size = decrypt_utils::read_uint_32_big_end(file_data, offset);
    type = decrypt_utils::read_box_type(file_data, offset);

    if (offset + size > file_data.size())
        size = file_data.size() - offset; // # 防止越界

    data = vector(file_data.begin() + offset + 8, file_data.begin() + offset + size);
}

bool mp4_box::empty() const
{
    if (size == 999 && offset == 999 && type == "999")
        return true;
    return false;
}

mp4_box mp4_box::find_box(const vector<uint8_t>& file_data, const string& box_type, size_t offset, size_t end)
{
    if (end == SIZE_MAX)
        end = file_data.size();
    size_t pos = offset;

    while (pos < end)
    {
        if (pos + 8 > end)
            break;

        const uint32_t size = decrypt_utils::read_uint_32_big_end(file_data, pos);
        if (size < 8 || size > end - pos)
            break;

        const string current_type = decrypt_utils::read_box_type(file_data, pos);
        if (current_type == box_type)
        {
            // # 找到目标box
            return mp4_box(file_data, pos);
        }
        pos += size;
    }
    return {999, "999", 999, {}};
}
