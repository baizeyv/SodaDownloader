#include <iostream>
#include <curl/curl.h>

#include "src/decryptor.h"
#include "src/cxxopts.h"
#include "src/lyric_convertor.h"
#include "src/requester.h"
#include "src/utils.h"
using namespace std;

int main(int argc, char* argv[])
{
    cxxopts::Options options(
        "soda", "\n[Soda Music Downloader]\n@author: baizeyv\n@contact: baizeyv@gmail.com\n@git: https://github.com/baizeyv/SodaDownloader\n");
    options.add_options()
        ("a,aid", "set aid.", cxxopts::value<std::string>()->default_value(""))
        ("s,sessionid", "set session id.", cxxopts::value<std::string>()->default_value(""))
        ("o,output", "output path.", cxxopts::value<std::string>()->default_value(""))
        ("l,link", "shared link.", cxxopts::value<std::string>()->default_value(""))
        ("v,version", "Show soda version.");

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
    
    try
    {
        const auto result = options.parse(argc, argv);
        if (result.count("help"))
        {
            std::cout << options.help() << std::endl;
            return 0;
        }
        if (result.count("output"))
        {
            output = result["output"].as<std::string>();
            utils::write_json(root, {{"aid", aid}, {"sessionid", session_id}, {"output", output}});
        }
        if (result.count("sessionid"))
        {
            session_id = result["sessionid"].as<std::string>();
            utils::write_json(root, {{"aid", aid}, {"sessionid", session_id}, {"output", output}});
        }
        if (result.count("aid"))
        {
            aid = result["aid"].as<std::string>();
            utils::write_json(root, {{"aid", aid}, {"sessionid", session_id}, {"output", output}});
        }
        if (result.count("link"))
        {
            string link = result["link"].as<std::string>();
            if (output.empty())
            {
                cout << "请使用 -o 配置输出目录!" << endl;
                return 0;
            }
            if (aid.empty())
            {
                cout << "请使用 -a 配置aid!" << endl;
                return 0;
            }
            if (session_id.empty())
            {
                cout << "请使用 -s 配置session id!" << endl;
                return 0;
            }
            curl_global_init(CURL_GLOBAL_ALL);
            link = utils::extract_link(link);
            if (!link.empty())
            {
                media_info media;
                requester::request_shared(link, media);
                requester::request_track_v2("https://api.qishui.com/luna/pc/track_v2", aid, session_id, media);
                
                media.print_list();
                cout << "please select id: ";
                string input_content;
                while (input_content.empty())
                {
fail:
                    std::getline(std::cin, input_content);
                }
                int id;
                stringstream ss(input_content);
                ss >> id;
                
                if (ss.fail())
                {
                    cout << "input error." << endl;
                    goto fail;
                }
                
                if (id < 0 || id >= media.model_map.size())
                {
                    cout << "input error." << endl;
                    goto fail;
                }
                
                auto model = media.model_map[id];
                
                requester::request_media(model.main_url);
                decryptor dt;
                dt.decrypt(model, media, output);
                lyric_convertor::save_lrc(output, media);
            } 
            else
            {
                cerr << "shared link error!" << endl;
            }
        }
    
    }
    catch (const cxxopts::exceptions::exception e)
    {
        cerr << "cxxopts error: " << e.what() << endl;
    }

    curl_global_cleanup();
    return 0;
}
