#include <iostream>

#include "include/initializer.h"
#include "include/requester.h"

#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

#include "include/decryptor.h"
#include "include/lyric_convertor.h"
#include "include/utils.h"

using namespace std;

int main(int argc, char* argv[])
{
    auto root = utils::get_save_root();
    utils::ensure_directory(root);
    auto js = utils::read_json(root);
    
    string output;
    string aid;
    string session_id;

    // # 输出目录
    if (!js.is_null() && !js.empty())
    {
        output = js.value("output", "");
        aid = js.value("aid", "");
        session_id = js.value("sessionid", "");
    }

    string shared_link;

    if (argc > 1)
    {
        // # 第一个参数
        const string cmd = argv[1];
        if (cmd == "--aid")
        {
            utils::write_json(root, {{"aid", argv[2]}, {"sessionid", session_id}, {"output", output}});
            return 0;
        }
        else if (cmd == "--sid")
        {
            utils::write_json(root, {{"aid", aid}, {"sessionid", argv[2]}, {"output", output}});
            return 0;
        }
        else if (cmd == "--output")
        {
            utils::write_json(root, {{"aid", aid}, {"sessionid", session_id}, {"output", argv[2]}});
            return 0;
        }
        else
        {
            shared_link = argv[1];
        }
    }

    if (output.empty())
    {
        cout << "请先配置输出目录!" << endl;
        return 0;
    }
    if (aid.empty())
    {
        cout << "请先配置aid!" << endl;
        return 0;
    }
    if (session_id.empty())
    {
        cout << "请先配置session id!" << endl;
        return 0;
    }

    if (js.is_null() || js.empty())
    {
        cout << "没有任何存储的信息,请使用部分参数进行配置." << endl;
        return 0;
    }

    initializer::setup();

    const auto link = utils::extract_link(shared_link);

    requester req;
    req.request_shared(link);
    auto p = req.inject_shared_parser();
    req.request_track_v2("https://api.qishui.com/luna/pc/track_v2", "386088", "bfd83ccb20ba2e5e9a5f5adba594ecf8", p.get_track_id());
    p = req.inject_track_v2_parser();
    // # 最优的音乐信息
    auto music_info = p.get_music_info();
    if (music_info != nullptr)
    {
        requester::request_media(music_info->main_url);
        cout << "[*] download encrypted file successful!" << endl;
        cout << "spade_a: " << music_info->spade_a << endl;
        decryptor::decrypt(music_info->spade_a, output, p);
    }
    else
    {
        cerr << "[!] music info is null." << endl;
    }
    
    lyric_convertor::save_lrc(output, p);

    initializer::dispose();
    
    // const auto tmp_file = root.string() + "/download.tmp";
    // if (fs::exists(tmp_file))
    //     fs::remove(tmp_file);
    
    return 0;
}
