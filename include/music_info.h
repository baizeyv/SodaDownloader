//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_MUSIC_INFO_H
#define SODADOWNLOADER_MUSIC_INFO_H
#include <string>

struct music_info
{
    /**
     * * 加密文件下载链接
     */
    std::string main_url;

    /**
     * * 用于 CENC 解密的key
     */
    std::string spade_a;

    /**
     * * 音频比特率,用于评估质量
     */
    long bitrate;
};

#endif //SODADOWNLOADER_MUSIC_INFO_H