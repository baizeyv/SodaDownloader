//
// Created by baizeyv on 2/1/2026.
//

#include "../include/lyric_convertor.h"

#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

#include "../include/utils.h"

void lyric_convertor::save_lrc(const std::string& output_path, const parser& ps)
{
    const auto krc = ps.get_lyric_krc();
    if (krc.empty())
        return;
    const auto artist = ps.get_track_artist();
    const auto title = ps.get_track_title();
    const auto lrc = krc_to_lrc(ps.get_lyric_krc(), artist, title);
    const std::string outFile = output_path + "/" + title + " - " + artist + ".lrc";
    utils::write_to_file(outFile, lrc);
    std::cout << "导出歌词成功! path: "  + outFile << std::endl;
}

std::string lyric_convertor::krc_to_lrc(const std::string& krc_content, const std::string& artist, const std::string& title)
{
    std::stringstream lrc;
    std::regex lineRegex(R"(\[(\d+),\d+\](.*))"); // 匹配每行 [start,duration]<...>字
    std::regex wordRegex(R"(<\d+,\d+,\d+>([^<]+))"); // 匹配每个字 <offset,duration,0>字

    std::sregex_iterator lineIt(krc_content.begin(), krc_content.end(), lineRegex);
    std::sregex_iterator end;

    lrc << "[00:00.00]" << utils::convert_encoding(title, "GBK", "UTF-8") << " - " << utils::convert_encoding(artist, "GBK", "UTF-8") << "\n";
    for (; lineIt != end; ++lineIt)
    {
        int startMs = std::stoi((*lineIt)[1]); // 行开始时间，毫秒
        std::string wordsPart = (*lineIt)[2]; // 每个字的内容部分

        std::string lineText;
        std::sregex_iterator wordIt(wordsPart.begin(), wordsPart.end(), wordRegex);
        for (; wordIt != end; ++wordIt)
        {
            lineText += (*wordIt)[1].str(); // 拼接每个字
        }

        // 转换毫秒为 LRC 时间格式 [mm:ss.xx]
        int totalSeconds = startMs / 1000;
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;
        int centiseconds = (startMs % 1000) / 10;

        lrc << "[" << std::setw(2) << std::setfill('0') << minutes
            << ":" << std::setw(2) << std::setfill('0') << seconds
            << "." << std::setw(2) << std::setfill('0') << centiseconds
            << "]" << lineText << "\n";
    }

    return lrc.str();
}
