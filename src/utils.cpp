//
// Created by baizeyv on 2/1/2026.
//

#include "../include/utils.h"

#include <fstream>
#include <iostream>

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

vector<uint8_t> utils::readFile(const string& filename)
{
    ifstream ifs(filename, ios::binary);
    if (!ifs) throw runtime_error("无法打开文件: " + filename);
    return vector<uint8_t>((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());
}
