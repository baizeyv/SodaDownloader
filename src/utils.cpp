//
// Created by baizeyv on 2/3/2026.
//

#include "utils.h"

#include <fstream>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <iconv.h>
#include <iostream>
#include <regex>
#include <nlohmann/json.hpp>

fs::path utils::get_save_root()
{
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (!localAppData)
    {
        throw runtime_error("%LOCALAPPDATA% not found.");
    }

    return fs::path(localAppData) / "meowody" / "SodaDownloader";
}

string utils::extract_link(const string& link)
{
    pmr::smatch match;
    if (std::regex_search(link, match, std::regex("https://[a-zA-Z0-9./]+")))
        return match[0];
    return "";
}

void utils::replace_all(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t start_pos = 0;
    // # 循环查找目标字符串
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        // # 执行替换
        str.replace(start_pos, from.length(), to);
        // # 移动起始位置，防止死循环（虽然在此场景下不会，但这是通用做法）
        start_pos += to.length();
    }
}

bool utils::write_to_file(const std::string& file_path, const std::string& content, bool append)
{
    std::ofstream ofs;

    if (append)
    {
        ofs.open(file_path, std::ios::out | std::ios::app);
    }
    else
    {
        ofs.open(file_path, std::ios::out | std::ios::trunc);
    }

    if (!ofs.is_open())
    {
        cerr << "无法打开文件: " << file_path << std::endl;
        return false;
    }

    ofs << content;

    // 自动关闭 ofs 析构时会关闭
    return true;
}

vector<uint8_t> utils::read_file(const string& path)
{
    ifstream f(path, ios::binary | ios::ate);
    if (!f) throw runtime_error("无法打开文件");
    const streamsize size = f.tellg();
    f.seekg(0, ios::beg);
    vector<uint8_t> buffer(size);
    if (!f.read(reinterpret_cast<char*>(buffer.data()), size)) throw runtime_error("读取文件失败");
    return buffer;
}

void utils::write_file(const string& path, const vector<uint8_t>& data)
{
    ofstream f(path, ios::binary);
    if (!f.write((char*)data.data(), data.size()))
        throw runtime_error("write file failed.");
}

bool utils::write_media_metadata(const string& file_path, const string& title, const string& artist, const string& album)
{
    TagLib::FileRef f(file_path.c_str());
    if (!f.isNull() && f.tag())
    {
        TagLib::Tag* tag = f.tag();

        tag->setTitle(TagLib::String(convert_encoding(title, "GBK", "UTF-8"), TagLib::String::UTF8));
        tag->setArtist(TagLib::String(convert_encoding(artist, "GBK", "UTF-8"), TagLib::String::UTF8));
        tag->setAlbum(TagLib::String(convert_encoding(album, "GBK", "UTF-8"), TagLib::String::UTF8));

        return f.save();
    }
    return false;
}

void utils::write_json(const fs::path& dir, const map<string, string>& json_map)
{
    const fs::path file = dir / "archive.json";
    nlohmann::json j = json_map;
    const auto str = j.dump();
    std::ofstream ofs(file);
    ofs << str;
}

nlohmann::json utils::read_json(const fs::path& dir)
{
    const fs::path file = dir / "archive.json";

    if (!fs::exists(file))
        return {};

    std::ifstream ifs(file, std::ios::in);
    if (!ifs)
        return {};

    std::stringstream buffer;
    buffer << ifs.rdbuf();

    try
    {
        return nlohmann::json::parse(buffer.str());
    }
    catch (const nlohmann::json::parse_error&)
    {
        // JSON 损坏
        return {};
    }
}

void utils::ensure_directory(const fs::path& dir)
{
    if (!fs::exists(dir))
    {
        fs::create_directories(dir); // 递归创建
    }
}

string utils::convert_encoding(const std::string& input, const std::string& from_encoding, const std::string& to_encoding)
{
    iconv_t cd = iconv_open(to_encoding.c_str(), from_encoding.c_str());
    if (cd == (iconv_t)-1)
    {
        return input;
    }

    size_t in_bytes = input.size();
    size_t out_bytes = in_bytes * 4; // 足够大的缓冲区
    char* in_buf = const_cast<char*>(input.data());
    char* out_buf = new char[out_bytes];
    char* out_ptr = out_buf;

    memset(out_buf, 0, out_bytes);

    if (iconv(cd, &in_buf, &in_bytes, &out_ptr, &out_bytes) == (size_t)-1)
    {
        delete[] out_buf;
        iconv_close(cd);
        return input;
    }

    std::string result(out_buf, out_ptr - out_buf);
    delete[] out_buf;
    iconv_close(cd);

    return result;
}

void utils::ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }));
}

void utils::rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
    {
        return !std::isspace(ch);
    }).base(), s.end());
}

void utils::trim(std::string& s)
{
    ltrim(s);
    rtrim(s);
}
