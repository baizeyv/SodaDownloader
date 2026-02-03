//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_MEDIA_INFO_H
#define SODADOWNLOADER_MEDIA_INFO_H

#include <string>
#include <vector>

#include "model_info.h"

using namespace std;

struct media_info
{
    string track_id;
    
    string title;
    
    string artist;
    
    string album;
    
    string lyric_krc;
    
    /**
     * * 所有质量的集合,用于后续筛选
     */
    vector<model_info> model_map{};
    
    void print_list() const;
};


#endif //SODADOWNLOADER_MEDIA_INFO_H