//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_REQUESTER_H
#define SODADOWNLOADER_REQUESTER_H
#include <map>
#include <string>
#include <curl/curl.h>

#include "parser.h"


class requester
{

    /**
     * * 请求的页面的内容
     */
    std::string shared_content;

    /**
     * * 请求track_v2的响应体
     */
    std::string track_v2_content;

    /**
     * * 转换器
     */
    parser p;

public:
    /**
     * * 请求分享页面的内容
     * @param p_url 
     */
    void request_shared(const std::string& p_url);

    /**
     * * 注入分享页面的转换器
     */
    [[nodiscard]]
    parser inject_shared_parser();

    /**
     * * 请求汽水音乐的解析api
     * @param p_url 
     * @param p_aid 
     * @param p_session_id 
     */
    void request_track_v2(const std::string& p_url, const std::string& p_aid, const std::string& p_session_id, const std::string& p_track_id);

    /**
     * * 注入track_v2响应体转换器
     * @return 
     */
    parser inject_track_v2_parser();

    /**
     * * 请求media
     * @param p_url 
     */
    static void request_media(const std::string& p_url);

    /**
     * * 发送解密请求
     * @param file_path 
     * @param spade_a 
     */
    static void request_decrypt(const std::string& file_path, const std::string& spade_a);

private:
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
     * * 构造请求参数
     * @param curl 
     * @param params 
     * @return 
     */
    static std::string build_query_params(CURL* curl, const std::map<std::string, std::string>& params);
};


#endif //SODADOWNLOADER_REQUESTER_H
