//
// Created by baizeyv on 2/1/2026.
//

#include "../include/parser.h"

#include <iostream>
#include <ostream>
#include <lexbor/html/parser.h>
#include <lexbor/html/interfaces/element.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>
#include <nlohmann/json.hpp>

#include "../include/utils.h"

void parser::inject_shared_page(const std::string& html_content)
{
    lxb_html_parser_t* parser = lxb_html_parser_create();
    lxb_status_t status = lxb_html_parser_init(parser);
    if (status != LXB_STATUS_OK) return;

    lxb_html_document_t* document = lxb_html_parse(parser, reinterpret_cast<const uint8_t*>(html_content.c_str()), html_content.length());

    // # 从根节点开始遍历寻找
    auto data = find_router_data(lxb_dom_interface_node(document->body));

    std::string router_data_dirty(data);

    lxb_html_document_destroy(document);
    lxb_html_parser_destroy(parser);

    auto router_data = clean_json_content(router_data_dirty);

    auto json_content = nlohmann::json::parse(router_data);
    track_id = json_content["loaderData"]["track_page"]["track_id"];
    track_title = json_content["loaderData"]["track_page"]["audioWithLyricsOption"]["trackName"];
    track_artist = json_content["loaderData"]["track_page"]["audioWithLyricsOption"]["artistName"];

    track_title = utils::convert_encoding(track_title, "UTF-8", "GBK");
    track_artist = utils::convert_encoding(track_artist, "UTF-8", "GBK");

    std::cout << "track_id: " << track_id << " title: " << track_title << " artist: " << track_artist << std::endl;
}

void parser::inject_track_v2(const std::string& track_v2_content)
{
    auto track_json_data = nlohmann::json::parse(track_v2_content);
    album = track_json_data["track"]["album"]["name"];
    album = utils::convert_encoding(album, "UTF-8", "GBK");
    lyric_krc = track_json_data["lyric"]["content"];
    video_model = track_json_data["track_player"]["video_model"];

    auto model_json_data = nlohmann::json::parse(video_model);
    auto list = model_json_data["video_list"];
    if (list.is_array())
    {
        for (const auto& item : list)
        {
            try
            {
                music_info info;
                info.main_url = item.value("main_url", "");
                if (item.contains("encrypt_info"))
                {
                    info.spade_a = item["encrypt_info"].value("spade_a", "");
                }
                if (item.contains("video_meta"))
                {
                    info.bitrate = item["video_meta"].value("bitrate", 0);
                }
                model_map.push_back(info);
            }
            catch (std::exception& e)
            {
                std::cerr << "解析单元出错: " << e.what() << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "video_model error!" << std::endl;
    }
}

std::string parser::get_track_id() const
{
    return track_id;
}

std::string parser::get_track_title() const
{
    return track_title;
}

std::string parser::get_track_artist() const
{
    return track_artist;
}

const music_info* parser::get_music_info() const
{
    const music_info* info = nullptr;
    for (const auto& item : model_map)
    {
        if (item.bitrate > 0 && !item.main_url.empty() && !item.spade_a.empty())
        {
            if (info == nullptr)
                info = &item;
            else if (item.bitrate > info->bitrate)
                info = &item;
        }
    }
    return info;
}

const char* parser::find_router_data(lxb_dom_node_t* node)
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

            if (attr_val && std::string((const char*)attr_val, len) == "modern-inline")
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

std::string parser::clean_json_content(const std::string& data_dirty)
{
    // 1. 找到第一个 '{'，这是 JSON 对象的开始
    size_t start = data_dirty.find('{');
    if (start == std::string::npos) return "";

    // 2. 从 start 开始遍历，计算大括号的匹配情况
    int brace_count = 0;
    size_t end = std::string::npos;

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

    if (end != std::string::npos)
    {
        // # 截取从第一个 '{' 到最后一个 '}' 的内容
        return data_dirty.substr(start, end - start + 1);
    }

    return "";
}
