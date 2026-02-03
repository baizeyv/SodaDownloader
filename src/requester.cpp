//
// Created by baizeyv on 2/3/2026.
//


#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <lexbor/html/parser.h>
#include <lexbor/html/interfaces/element.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>

#include "utils.h"

#include "requester.h"

size_t requester::write_callback(void* contents, const size_t size, const size_t nmemb, string* userp)
{
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

size_t requester::write_media_callback(void* contents, size_t size, size_t nmemb, FILE* userp)
{
    const size_t written = fwrite(contents, size, nmemb, userp);
    return written;
}

const char* requester::find_router_data(lxb_dom_node_t* node)
{
    lxb_dom_element_t* element = lxb_dom_interface_element(node);

    // 1. 检查是否为 <script> 标签
    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT && element->node.local_name == LXB_TAG_SCRIPT)
    {
        // 2. 检查是否有 async 属性
        if (lxb_dom_element_has_attribute(element, (const uint8_t*)"async", 5))
        {
            // 3. 检查 data-script-src 是否等于 "modern-inline"
            size_t len;
            const uint8_t* attr_val = lxb_dom_element_get_attribute(element, (const uint8_t*)"data-script-src", 15, &len);

            if (attr_val && string((const char*)attr_val, len) == "modern-inline")
            {
                // 4. 获取内部文本内容 (Child Node)
                lxb_dom_node_t* child = lxb_dom_node_first_child(node);
                if (child && child->type == LXB_DOM_NODE_TYPE_TEXT)
                {
                    lxb_dom_text_t* text = lxb_dom_interface_text(child);
                    return (const char*)text->char_data.data.data;
                }
            }
        }
    }

    // 递归遍历子节点
    for (lxb_dom_node_t* child = lxb_dom_node_first_child(node); child; child = lxb_dom_node_next(child))
    {
        if (const auto result = find_router_data(child); result != nullptr)
            return result;
    }
    return nullptr;
}

string requester::clean_json_content(const string& data_dirty)
{
    // 1. 找到第一个 '{'，这是 JSON 对象的开始
    size_t start = data_dirty.find('{');
    if (start == string::npos) return "";

    // 2. 从 start 开始遍历，计算大括号的匹配情况
    int brace_count = 0;
    size_t end = string::npos;

    for (size_t i = start; i < data_dirty.length(); ++i)
    {
        if (data_dirty[i] == '{')
        {
            brace_count++;
        }
        else if (data_dirty[i] == '}')
        {
            brace_count--;
        }

        // 当计数回到 0 时，说明找到了最外层匹配的右大括号
        if (brace_count == 0)
        {
            end = i;
            break;
        }
    }

    if (end != string::npos)
    {
        // # 截取从第一个 '{' 到最后一个 '}' 的内容
        return data_dirty.substr(start, end - start + 1);
    }

    return "";
}

int requester::download_progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    if (dltotal <= 0) return 0; // 还没获取到文件大小时直接返回

    // 计算百分比
    double fraction = static_cast<double>(dlnow) / static_cast<double>(dltotal);
    int percentage = static_cast<int>(fraction * 100);

    // 在控制台打印进度条
    // \r 会让光标回到行首，实现原地刷新的效果
    std::cout << "[INF] Downloading: " << percentage << "% ("
        << (dlnow / 1024) << " KB / " << (dltotal / 1024) << " KB)\r";
    std::cout.flush(); // 强制刷新缓冲区

    return 0; // 返回非 0 值会中断下载
}

void requester::request_shared(const string& url, media_info& info)
{
    // # 模拟浏览器请求html
    CURL* curl = curl_easy_init();
    string shared_content;
    if (curl)
    {
        // # 模拟浏览器请求 HTML
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
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
            cerr << "访问页面失败: " << curl_easy_strerror(res) << endl;
        }
        else
        {
            cout << "访问分享页面成功!" << endl;
        }
    }
    else
    {
        cerr << "curl_easy_init() failed" << endl;
    }
    curl_easy_cleanup(curl);
    if (shared_content.empty())
        return;

    // # 解析 _ROUTER_DATA
    lxb_html_parser_t* parser = lxb_html_parser_create();
    lxb_status_t status = lxb_html_parser_init(parser);
    if (status != LXB_STATUS_OK) return;

    lxb_html_document_t* document = lxb_html_parse(parser, reinterpret_cast<const uint8_t*>(shared_content.c_str()), shared_content.length());

    // # 从根节点开始遍历寻找
    const auto data = find_router_data(lxb_dom_interface_node(document->body));

    const string router_data_dirty(data);

    lxb_html_document_destroy(document);
    lxb_html_parser_destroy(parser);

    auto router_data = clean_json_content(router_data_dirty);

    auto json_content = nlohmann::json::parse(router_data);
    const string track_id = json_content["loaderData"]["track_page"]["track_id"];
    string track_title = json_content["loaderData"]["track_page"]["audioWithLyricsOption"]["trackName"];
    string track_artist = json_content["loaderData"]["track_page"]["audioWithLyricsOption"]["artistName"];

    track_title = utils::convert_encoding(track_title, "UTF-8", "GBK");
    track_artist = utils::convert_encoding(track_artist, "UTF-8", "GBK");
    utils::replace_all(track_title, "/", "&");
    utils::replace_all(track_artist, "/", "&");
    utils::replace_all(track_title, ",", "");
    utils::replace_all(track_artist, ",", "");
    utils::replace_all(track_title, "，", "");
    utils::replace_all(track_artist, "，", "");

    utils::trim(track_artist);
    utils::trim(track_title);
    info.track_id = track_id;
    info.artist = track_artist;
    info.title = track_title;

    cout << "track_id: " << track_id << " title: " << track_title << " artist: " << track_artist << endl;
}

void requester::request_track_v2(const string& url, const string& p_aid, const string& p_session_id, media_info& info)
{
    // # 模拟浏览器请求html
    CURL* curl = curl_easy_init();
    string track_v2_content;
    if (curl)
    {
        const string full_url = url + "?aid=" + p_aid;

        const nlohmann::json payload = {
            {"track_id", info.track_id},
            {"media_type", "track"},
            {"queue_type", "search_one_track"},
            {"scene_name", "search"}
        };

        string json_data = payload.dump();

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
            cout << "请求track_v2成功!" << endl;
        }

        curl_slist_free_all(headers);
    }
    else
    {
        cerr << "curl_easy_init() failed" << endl;
    }
    curl_easy_cleanup(curl);

    // *******************************************************************
    // # 解析响应体
    auto track_json_data = nlohmann::json::parse(track_v2_content);
    string album = track_json_data["track"]["album"]["name"];
    album = utils::convert_encoding(album, "UTF-8", "GBK");
    string lyric_krc;
    if (track_json_data.contains("lyric") && track_json_data["lyric"].contains("content"))
        lyric_krc = track_json_data["lyric"]["content"];
    string video_model = track_json_data["track_player"]["video_model"];

    info.album = album;
    info.lyric_krc = lyric_krc;

    auto model_json_data = nlohmann::json::parse(video_model);
    auto list = model_json_data["video_list"];
    if (list.is_array())
    {
        for (const auto& item : list)
        {
            try
            {
                model_info mif;
                mif.main_url = item.value("main_url", "");
                if (item.contains("encrypt_info"))
                {
                    mif.spade_a = item["encrypt_info"].value("spade_a", "");
                }
                if (item.contains("video_meta"))
                {
                    mif.bitrate = item["video_meta"].value("bitrate", 0);
                    mif.codec_type = item["video_meta"].value("codec_type", "unknown");
                }
                info.model_map.push_back(mif);
            }
            catch (exception& e)
            {
                cerr << "解析单元出错: " << e.what() << endl;
            }
        }
    }
    else
    {
        cerr << "video_model error!" << endl;
    }
}

void requester::request_media(const string& p_url)
{
    CURL* curl = curl_easy_init();
    if (!curl) return;

    const auto root = utils::get_save_root().string();
    const auto out_filename = root + "/download.tmp";

    try
    {
        // 2. 自动创建目录
        const filesystem::path p(out_filename);
        const filesystem::path dir = p.parent_path();
        if (!dir.empty() && !filesystem::exists(dir))
        {
            filesystem::create_directories(dir); // 相当于 mkdir -p
        }

        // 3. 处理旧文件：如果存在则删除 (满足你“先删除再写”的需求)
        if (filesystem::exists(p))
        {
            filesystem::remove(p);
        }
    }
    catch (const filesystem::filesystem_error& e)
    {
        cerr << "文件系统预处理失败: " << e.what() << endl;
        curl_easy_cleanup(curl);
        return;
    }

    // 以二进制写入模式打开文件
    FILE* fp = fopen(out_filename.c_str(), "wb");
    if (!fp)
    {
        cerr << "无法创建文件: " << out_filename << endl;
        curl_easy_cleanup(curl);
        return;
    }

    // 设置请求选项
    curl_easy_setopt(curl, CURLOPT_URL, p_url.c_str());

    // 设置回调函数和写入目标
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_media_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L); // 0 表示开启进度功能
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, download_progress_callback);

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
        cerr << "下载失败: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_cleanup(curl);
}
