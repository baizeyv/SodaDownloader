//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_UTILS_H
#define SODADOWNLOADER_UTILS_H
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <nlohmann/adl_serializer.hpp>

#include "box.h"

using namespace std;

namespace fs = std::filesystem;

class utils
{
public:
    /**
     * * 提取链接
     * @param link 
     * @return 
     */
    static string extract_link(const string& link);
    
    /**
     * * 写文件
     * @param file_path 
     * @param content 
     * @param append 
     * @return 
     */
    static bool write_to_file(const std::string& file_path, const std::string& content, bool append = false);

    /**
     * * 替换字符串
     * @param str 
     * @param from 
     * @param to 
     */
    static void replace_all(std::string& str, const std::string& from, const std::string& to);

    /**
     * * 读取文件
     * @param path 
     * @return 
     */
    static vector<uint8_t> readFile(const string& path);

    /**
     * * 写文件
     * @param path 
     * @param data 
     */
    static void writeFile(const string& path, const vector<uint8_t>& data);


    /**
     * * 读取 uint32_t
     * @param data 
     * @param offset 
     * @return 
     */
    static uint32_t readUint32BE(const vector<uint8_t>& data, size_t offset);
    
    
    static std::vector<uint8_t> writeUint32BE(uint32_t value);

    static void writeUint32BE(uint8_t* buf, uint32_t value);
    
    /**
     * * 16进制转bytes
     * @param hex 
     * @return 
     */
    static vector<uint8_t> hexToBytes(const string& hex);
    
    static string readBoxType(const std::vector<uint8_t>& data, size_t offset);
    
    static std::vector<uint8_t> concatUint8Arrays(const std::vector<std::vector<uint8_t>>& arrays);

    /**
     * * 获取存档的根目录
     * @return 
     */
    static fs::path get_save_root();

    /**
     * * 安全创建目录
     * @param dir 
     */
    static void ensure_directory(const fs::path& dir);

    /**
     * * 写json文件
     * @param dir 
     * @param json_map 
     */
    static void write_json(const fs::path& dir, const std::map<string, string>& json_map);

    /**
     * * 读json文件
     * @param dir 
     * @return 
     */
    static nlohmann::json read_json(const fs::path& dir);
    
    static std::string convert_encoding(const std::string& input, 
                             const std::string& from_encoding, 
                             const std::string& to_encoding) ;

    /**
     * * 获取 decrypt_m4a.exe 要保存的路径
     * @return 
     */
    static string get_m4a_decrypt_exe_path();
};


#endif //SODADOWNLOADER_UTILS_H
