//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_PARSER_H
#define SODADOWNLOADER_PARSER_H
#include <string>
#include <unordered_map>
#include <lexbor/dom/interface.h>

#include "music_info.h"


class parser
{
    /**
     * * 音轨id
     */
    std::string track_id;

    /**
     * * 歌曲名称
     */
    std::string track_title;

    /**
     * * 演唱
     */
    std::string track_artist;

    /**
     * * 专辑
     */
    std::string album;

    /**
     * * 原始的lyric (尚未转为lrc, 目前是krc)
     */
    std::string lyric_krc;

    /**
     * * 要解析的音乐的每个质量的信息
     */
    std::string video_model;

    /**
     * * 所有质量的集合,用于后续筛选
     */
    std::vector<music_info> model_map;
    
public:
    /**
     * * 注入分享页面的内容
     * @param html_content 页面内容
     */
    void inject_shared_page(const std::string& html_content);

    /**
     * * 注入 track_v2 的响应体
     * @param track_v2_content 
     */
    void inject_track_v2(const std::string& track_v2_content);

    /**
     * * 获取音频轨道id
     * @return 
     */
    [[nodiscard]] 
    std::string get_track_id() const;

    /**
     * * 获取最优歌曲信息
     * @return 
     */
    [[nodiscard]]
    const music_info* get_music_info() const;
    
private:
    
    /**
     * * 查找 router data 内容
     * @param node 
     * @return 
     */
    static const char* find_router_data(lxb_dom_node_t* node);

    /**
     * * 清理json中的脏数据部分
     * @param data_dirty 
     * @return 
     */
    static std::string clean_json_content(const std::string& data_dirty);
};


#endif //SODADOWNLOADER_PARSER_H