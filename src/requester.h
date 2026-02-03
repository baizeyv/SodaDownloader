//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_REQUESTER_H
#define SODADOWNLOADER_REQUESTER_H

#include <string>
#include <lexbor/dom/interface.h>

#include "media_info.h"

using namespace std;

class requester
{
    /**
     * * 回调函数：处理字符串流
     * @param contents 
     * @param size 
     * @param nmemb 
     * @param userp 
     * @return 
     */
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp);
    /**
     * * 回调函数：处理字符串流
     * @param contents 
     * @param size 
     * @param nmemb 
     * @param userp 
     * @return 
     */
    static size_t write_media_callback(void* contents, size_t size, size_t nmemb, FILE* userp);
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
    static string clean_json_content(const std::string& data_dirty);
    
    // 进度回调函数
    // dltotal: 文件总大小 (字节)
    // dlnow:   当前已下载大小 (字节)
    static int download_progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

public:
    static void request_shared(const string& url, media_info& info);
    
    static void request_track_v2(const string& url, const string& p_aid, const string& p_session_id, media_info& info);
    
    /**
     * * 请求media
     * @param p_url 
     */
    static void request_media(const std::string& p_url);
};


#endif //SODADOWNLOADER_REQUESTER_H
