//
// Created by baizeyv on 2/3/2026.
//

#include "model_info.h"

string model_info::get_str() const
{
    string ret;
    ret += to_string(bitrate);
    ret += "\t";
    ret += codec_type;
    return ret;
}
