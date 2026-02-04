//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_MODEL_INFO_H
#define SODADOWNLOADER_MODEL_INFO_H

#include <string>

using namespace std;

struct model_info
{
    string main_url;
    
    string spade_a;
    
    string codec_type;
    
    long bitrate;
    
    string get_str() const;
};


#endif //SODADOWNLOADER_MODEL_INFO_H