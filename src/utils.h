//
// Created by baizeyv on 2/3/2026.
//

#ifndef SODADOWNLOADER_UTILS_H
#define SODADOWNLOADER_UTILS_H
#include <filesystem>

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
    static vector<uint8_t> read_file(const string& path);
    
    /**
     * * 写文件
     * @param path 
     * @param data 
     */
    static void write_file(const string& path, const vector<uint8_t>& data);
    
    static bool write_media_metadata(const string& file_path, const string& title, const string& artist, const string& album);
};


#endif //SODADOWNLOADER_UTILS_H