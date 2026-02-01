//
// Created by baizeyv on 2/1/2026.
//

#include "../include/utils.h"

#include <fstream>
#include <iconv.h>
#include <iostream>
#include <regex>
#include <nlohmann/json.hpp>

string utils::extract_link(const string& link)
{
    smatch match;
    if (std::regex_search(link, match, std::regex("https://[a-zA-Z0-9./]+")))
        return match[0];
    return "";
}

bool utils::write_to_file(const std::string& file_path, const std::string& content, const bool append)
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
        std::cerr << "无法打开文件: " << file_path << std::endl;
        return false;
    }

    ofs << content;

    // 自动关闭 ofs 析构时会关闭
    return true;
}

void utils::replace_all(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t start_pos = 0;
    // 循环查找目标字符串
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        // 执行替换
        str.replace(start_pos, from.length(), to);
        // 移动起始位置，防止死循环（虽然在此场景下不会，但这是通用做法）
        start_pos += to.length();
    }
}

vector<uint8_t> utils::readFile(const string& path)
{
    ifstream f(path, ios::binary | ios::ate);
    if (!f) throw runtime_error("无法打开文件");
    const streamsize size = f.tellg();
    f.seekg(0, ios::beg);
    vector<uint8_t> buffer(size);
    if (!f.read(reinterpret_cast<char*>(buffer.data()), size)) throw runtime_error("读取文件失败");
    return buffer;
}

void utils::writeFile(const string& path, const vector<uint8_t>& data)
{
    ofstream f(path, ios::binary);
    if (!f.write((char*)data.data(), data.size()))
        throw runtime_error("写入文件失败");
}

uint32_t utils::readUint32BE(const vector<uint8_t>& data, size_t offset)
{
    return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3];
}

std::vector<uint8_t> utils::writeUint32BE(uint32_t value)
{
    std::vector<uint8_t> bytes(4);
    bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[3] = static_cast<uint8_t>(value & 0xFF);
    return bytes;
}

void utils::writeUint32BE(uint8_t* buf, uint32_t value)
{
    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    buf[3] = value & 0xFF;
}

vector<uint8_t> utils::hexToBytes(const string& hex)
{
    vector<uint8_t> bytes;
    if (hex.size() % 2 != 0) throw runtime_error("Hex 长度必须为偶数");
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        uint8_t b = static_cast<uint8_t>(stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

string utils::readBoxType(const std::vector<uint8_t>& data, size_t offset)
{
    if (offset + 8 > data.size()) throw std::out_of_range("readBoxType 超出范围");
    char type[5] = {0};
    std::memcpy(type, &data[offset + 4], 4);
    return std::string(type);
}

std::vector<uint8_t> utils::concatUint8Arrays(const std::vector<std::vector<uint8_t>>& arrays)
{
    // 先计算总长度
    size_t totalLength = 0;
    for (const auto& arr : arrays) totalLength += arr.size();

    // 创建结果 vector
    std::vector<uint8_t> result;
    result.reserve(totalLength); // 提前分配内存

    // 拼接所有数组
    for (const auto& arr : arrays)
    {
        result.insert(result.end(), arr.begin(), arr.end());
    }

    return result;
}

fs::path utils::get_save_root()
{
    const char* localAppData = std::getenv("LOCALAPPDATA");
    if (!localAppData)
    {
        throw std::runtime_error("LOCALAPPDATA not found");
    }

    return fs::path(localAppData) / "meowody" / "SodaDownloader";
}

void utils::ensure_directory(const fs::path& dir)
{
    if (!fs::exists(dir))
    {
        fs::create_directories(dir); // 递归创建
    }
}

void utils::write_json(const fs::path& dir, const std::map<string, string>& json_map)
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

std::string utils::convert_encoding(const std::string& input, const std::string& from_encoding, const std::string& to_encoding)
{iconv_t cd = iconv_open(to_encoding.c_str(), from_encoding.c_str());
    if (cd == (iconv_t)-1) {
        return input;
    }
    
    size_t in_bytes = input.size();
    size_t out_bytes = in_bytes * 4; // 足够大的缓冲区
    char* in_buf = const_cast<char*>(input.data());
    char* out_buf = new char[out_bytes];
    char* out_ptr = out_buf;
    
    memset(out_buf, 0, out_bytes);
    
    if (iconv(cd, &in_buf, &in_bytes, &out_ptr, &out_bytes) == (size_t)-1) {
        delete[] out_buf;
        iconv_close(cd);
        return input;
    }
    
    std::string result(out_buf, out_ptr - out_buf);
    delete[] out_buf;
    iconv_close(cd);
    
    return result;
}

