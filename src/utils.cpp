//
// Created by baizeyv on 2/3/2026.
//

#include "utils.h"

#include <fstream>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

fs::path utils::get_save_root()
{
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (!localAppData)
    {
        throw runtime_error("%LOCALAPPDATA% not found.");
    }

    return fs::path(localAppData) / "meowody" / "SodaDownloader";
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
        
        tag->setTitle(TagLib::String(title, TagLib::String::UTF8));
        tag->setArtist(TagLib::String(artist, TagLib::String::UTF8));
        tag->setAlbum(TagLib::String(album, TagLib::String::UTF8));
        
        return f.save();
    }
    return false;
}
