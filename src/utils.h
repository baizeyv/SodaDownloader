//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_UTILS_H
#define SODADOWNLOADER_UTILS_H
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>

using namespace std;

namespace fs = std::filesystem;

class utils
{
public:
    /**
     * * 获取存档的根目录
     * @return 
     */
    static fs::path get_save_root();
    
    /**
     * * 提取链接
     * @param link 
     * @return 
     */
    static string extract_link(const string& link);

    /**
     * * 替换字符串
     * @param str 
     * @param from 
     * @param to 
     */
    static void replace_all(std::string& str, const std::string& from, const std::string& to);

    /**
     * * 写文件
     * @param file_path 
     * @param content 
     * @param append 
     * @return 
     */
    static bool write_to_file(const std::string& file_path, const std::string& content, bool append = false);
    /**
     * * 读取文件
     * @param path 
     * @return 
     */
    static vector<uint8_t> read_file(const string& path);

    /**
     * * 写文件
     * @param path 
     * @param data 
     */
    static void write_file(const string& path, const vector<uint8_t>& data);

    static bool write_media_metadata(const string& file_path, const string& title, const string& artist, const string& album);

    /**
     * * 写json文件
     * @param dir 
     * @param json_map 
     */
    static void write_json(const fs::path& dir, const map<string, string>& json_map);

    /**
     * * 读json文件
     * @param dir 
     * @return 
     */
    static nlohmann::json read_json(const fs::path& dir);

    /**
     * * 安全创建目录
     * @param dir 
     */
    static void ensure_directory(const fs::path& dir);
    
    static string convert_encoding(const std::string& input,
                                   const std::string& from_encoding,
                                   const std::string& to_encoding);
    static void ltrim(std::string &s);
    static void rtrim(std::string &s);
    static void trim(std::string &s);
};


#endif //SODADOWNLOADER_UTILS_H
