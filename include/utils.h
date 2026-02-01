//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_UTILS_H
#define SODADOWNLOADER_UTILS_H
#include <string>
#include <vector>

#include "box.h"
#include "mp4_box.h"

using namespace std;

class utils
{
public:
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
     * @param filename 
     * @return 
     */
    static vector<uint8_t> readFile(const string &filename);
    
};


#endif //SODADOWNLOADER_UTILS_H
