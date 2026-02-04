//
// Created by baizeyv on 2/3/2026.
//

#include "media_info.h"

#include <iostream>

using namespace std;

void media_info::print_list() const
{
    string str;
    str += "id\tbitrate\tcodec_type\n";
    for (auto i = 0; i < model_map.size(); ++i)
    {
        str += to_string(i);
        str += "\t";
        str += model_map[i].get_str();
        str += "\n";
    }
    cout << str;
}
