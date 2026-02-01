//
// Created by baizeyv on 2/1/2026.
//

#include "../include/requester.h"

#include <iostream>
#include <ostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "../include/utils.h"

void requester::request_shared(const std::string& p_url)
{
    // # 模拟浏览器请求html
    CURL* curl = curl_easy_init();
    if (curl)
    {
        // # 模拟浏览器请求 HTML
        curl_easy_setopt(curl, CURLOPT_URL, p_url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // 必须跟随重定向
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &shared_content);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        // 模拟 User-Agent 避免被拦截
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                         "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            std::cerr << "访问页面失败: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            std::cout << "访问分享页面成功!" << std::endl;
        }
    }
    else
    {
        std::cerr << "curl_easy_init() failed" << std::endl;
    }
    curl_easy_cleanup(curl);
}

parser requester::inject_shared_parser()
{
    p.inject_shared_page(shared_content);
    return p;
}

void requester::request_track_v2(const std::string& p_url, const std::string& p_aid, const std::string& p_session_id, const std::string& p_track_id)
{
    // # 模拟浏览器请求html
    CURL* curl = curl_easy_init();
    if (curl)
    {
        const std::string full_url = p_url + "?aid=" + p_aid;

        nlohmann::json payload = {
            {"track_id", p_track_id},
            {"media_type", "track"},
            {"queue_type", "search_one_track"},
            {"scene_name", "search"}
        };

        std::string json_data = payload.dump();

        // # 设置URL和cookie
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIE, ("sessionid=" + p_session_id).c_str());

        // # 设置为POST模式并填入数据
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

        // # 4. 设置http头(发送json必须设置)
        curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
        headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // # 配置回调获取响应
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &track_v2_content);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            std::cout << "请求track_v2成功!" << std::endl;
            utils::write_to_file("./test.json", track_v2_content);
        }

        curl_slist_free_all(headers);
    }
    else
    {
        std::cerr << "curl_easy_init() failed" << std::endl;
    }
    curl_easy_cleanup(curl);
}

parser requester::inject_track_v2_parser()
{
    p.inject_track_v2(track_v2_content);
    return p;
}

void requester::request_media(const std::string& p_url)
{
    CURL* curl = curl_easy_init();
    if (!curl) return;

    const auto root = utils::get_save_root().string();
    const auto out_filename = root + "/download.tmp";

    try
    {
        // 2. 自动创建目录
        std::filesystem::path p(out_filename);
        std::filesystem::path dir = p.parent_path();
        if (!dir.empty() && !std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir); // 相当于 mkdir -p
        }

        // 3. 处理旧文件：如果存在则删除 (满足你“先删除再写”的需求)
        if (std::filesystem::exists(p))
        {
            std::filesystem::remove(p);
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "文件系统预处理失败: " << e.what() << std::endl;
        curl_easy_cleanup(curl);
        return;
    }

    // 以二进制写入模式打开文件
    FILE* fp = fopen(out_filename.c_str(), "wb");
    if (!fp)
    {
        std::cerr << "无法创建文件: " << out_filename << std::endl;
        curl_easy_cleanup(curl);
        return;
    }

    // 设置请求选项
    curl_easy_setopt(curl, CURLOPT_URL, p_url.c_str());

    // 设置回调函数和写入目标
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_media_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    // 建议：处理重定向 (很多媒体 URL 会重定向)
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // 建议：设置 User-Agent 防止被某些服务器拒绝
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    // 执行下载
    const CURLcode res = curl_easy_perform(curl);

    // 清理工作
    fclose(fp);

    if (res != CURLE_OK)
    {
        std::cerr << "下载失败: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);
}

size_t requester::write_callback(void* contents, const size_t size, const size_t nmemb, std::string* userp)
{
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

size_t requester::write_media_callback(void* contents, size_t size, size_t nmemb, FILE* userp)
{
    const size_t written = fwrite(contents, size, nmemb, userp);
    return written;
}
