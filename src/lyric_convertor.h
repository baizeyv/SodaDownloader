//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_LYRIC_CONVERTOR_H
#define SODADOWNLOADER_LYRIC_CONVERTOR_H
#include <string>

#include "media_info.h"

class lyric_convertor
{
public:
    static void save_lrc(const std::string& output_path, const media_info& info);
private:
    static std::string krc_to_lrc(const std::string& krc_content, const std::string& artist, const std::string& title);
};


#endif //SODADOWNLOADER_LYRIC_CONVERTOR_H