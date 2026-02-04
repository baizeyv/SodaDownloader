//
// Created by baizeyv on 2/3/2026.
//

#include "decryptor.h"

#include <iostream>
#include <string>
#include <unordered_set>

#include "decrypt_utils.h"
#include "mp4_box.h"
#include "utils.h"

void decryptor::decrypt(const model_info& model, const media_info& media, const string& output_path)
{
    // # 临时目录
    const auto root = utils::get_save_root().string();
    // # 加密的文件路径
    string file_path = root + "/download.tmp";
    utils::replace_all(file_path, "\\", "/");

    // # 加密文件内容
    vector<uint8_t> file_data = utils::read_file(file_path);

    string key_hex = model.spade_a;
    if (model.spade_a.find_first_not_of("0123456789abcdefABCDEF") != string::npos)
    {
        key_hex = decrypt_utils::decrypt_spade_a(model.spade_a);
        if (key_hex.empty())
        {
            cerr << "spade_a key decrypt failed." << endl;
            return;
        }
    }
    cout << "[INF] hex key: " << key_hex << endl;
    vector<uint8_t> key_bytes = decrypt_utils::hex_to_bytes(key_hex);

    mp4_box moov = mp4_box::find_box(file_data, "moov");
    if (moov.empty())
    {
        cerr << "decrypt faild: not found 'moov atom'" << endl;
        return;
    }

    mp4_box trak = mp4_box::find_box(file_data, "trak", moov.offset + 8, moov.offset + moov.size);
    mp4_box mdia = mp4_box::find_box(file_data, "mdia", trak.offset + 8, trak.offset + trak.size);
    mp4_box minf = mp4_box::find_box(file_data, "minf", mdia.offset + 8, mdia.offset + mdia.size);
    mp4_box stbl = mp4_box::find_box(file_data, "stbl", minf.offset + 8, minf.offset + minf.size);
    // ! 关键：获取 stsd 以检查是否为 FLAC
    auto stsd = mp4_box::find_box(file_data, "stsd", stbl.offset + 8, stbl.offset + stbl.size);

    // # flac 元数据
    auto flac_metadata = decrypt_utils::scan_for_flac_metadata(stsd.data);
    bool is_flac = !flac_metadata.empty();
    if (is_flac)
    {
        cout << "[INF] FLAC encoding detected! Will be extracted as a '.flac' file." << endl;
    }
    else
    {
        cout << "[INF] FLAC not detected! maybe AAC/AVC, will be reconstructed as MP4/M4A." << endl;
    }

    mp4_box stsz = mp4_box::find_box(file_data, "stsz", stbl.offset + 8, stbl.offset + stbl.size);
    mp4_box stsc = mp4_box::find_box(file_data, "stsc", stbl.offset + 8, stbl.offset + stbl.size);
    mp4_box stco = mp4_box::find_box(file_data, "stco", stbl.offset + 8, stbl.offset + stbl.size);

    mp4_box senc = mp4_box::find_box(file_data, "senc", moov.offset + 8, moov.offset + moov.size);
    if (senc.empty())
    {
        senc = mp4_box::find_box(file_data, "senc", stbl.offset + 8, stbl.offset + stbl.size);
        cout << "[INF] found crypt info (senc) in stbl." << endl;
    }
    else
    {
        cout << "[INF] found crypt info (senc) in moov." << endl;
    }
    if (senc.empty())
    {
        cerr << "not found crypt info (senc)." << endl;
        return;
    }

    g_sample_sizes = decrypt_utils::parse_stsz(stsz.data);
    g_stsc_entries = decrypt_utils::parse_stsc(stsc.data);
    g_chunk_count = decrypt_utils::read_uint_32_big_end(stco.data, 4);
    auto ivs = decrypt_utils::parse_senc(senc.data);
    auto mdat = mp4_box::find_box(file_data, "mdat");

    cout << "[INF] sample count: " << g_sample_sizes.size() << ", iv count: " << ivs.size() << endl;
    cout << "[INF] start AES-CRT decrypt ..." << endl;
    vector<vector<uint8_t>> decrypted_samples;
    size_t sample_offset = mdat.offset + 8;
    for (size_t i = 0; i < g_sample_sizes.size(); ++i)
    {
        size_t size = g_sample_sizes[i];
        auto iv = ivs[i];
        vector encrypted(file_data.begin() + sample_offset, file_data.begin() + sample_offset + size);
        vector<uint8_t> dec = decrypt_utils::aes_ctr_decrypt(key_bytes, iv, encrypted);
        decrypted_samples.push_back(dec);
        sample_offset += size;
        if (i % 100 == 0)
            cout << "[INF] decrypt sample " << i << "/" << g_sample_sizes.size() << "\r";
    }

    // # 最终解密的文件的内容
    vector<uint8_t> final_data;
    // # 最终文件的后缀
    string final_ext;
    if (is_flac)
    {
        // # FLAC
        vector<uint8_t> flac_sig = {0x66, 0x4C, 0x61, 0x43}; // "fLaC"
        // # metadata body: 去掉前4字节 version+flags
        vector<uint8_t> meta_body;
        if (flac_metadata.size() > 4)
            meta_body.insert(meta_body.end(), flac_metadata.begin() + 4, flac_metadata.end());
        else
            meta_body = flac_metadata;
        vector<vector<uint8_t>> parts;
        parts.push_back(flac_sig);
        parts.push_back(meta_body);
        for (const auto& sample : decrypted_samples)
        {
            parts.push_back(sample);
        }
        final_data = decrypt_utils::concat_uint_8_arrays(parts);
        final_ext = ".flac";

        cout << "[INF] wrap as FLAC." << endl;
    }
    else
    {
        // # M4A
        size_t write_ptr = mdat.offset + 8;
        for (const auto& sample : decrypted_samples)
        {
            ranges::copy(sample, file_data.begin() + write_ptr);
            write_ptr += sample.size();
        }

        size_t search_start = stsd.offset;
        size_t search_end = stsd.offset + stsd.size;
        for (size_t i = search_start; i + 4 <= search_end; ++i)
        {
            if (file_data[i] == 'e' && file_data[i + 1] == 'n' && file_data[i + 2] == 'c' && file_data[i + 3] == 'a')
            {
                file_data[i] = 'm';
                file_data[i + 1] = 'p';
                file_data[i + 2] = '4';
                file_data[i + 3] = 'a';
                break;
            }
        }
        final_data = move(file_data);


        final_ext = ".m4a";
        cout << "[INF] wrap as M4A." << endl;
    }

    string out_path = output_path + "/[" + media.title + "] - " + media.artist + final_ext;
    utils::write_file(out_path, final_data);
    cout << "[INF] save file: " << out_path << endl;
    if (!utils::write_media_metadata(out_path, media.title, media.artist, media.album))
    {
        cerr << "[ERR] failed to write tag." << endl;
    }
}

vector<uint8_t> decryptor::process_box_tree(const vector<uint8_t>& data, size_t offset, size_t size, uint32_t new_mdat_offset)
{
    vector<vector<uint8_t>> parts;
    size_t pos = offset + 8; // # 跳过 box hearder
    size_t end = offset + size;

    pmr::unordered_set<string> skip_boxes = {"senc", "saio", "saiz", "sinf", "schi", "tenc", "schm", "frma"};
    unordered_set<string> container_boxes = {"moov", "trak", "mdia", "minf", "stbl", "stsd"};

    while (pos < end)
    {
        if (pos + 8 > end)
        {
            parts.push_back(vector(data.begin() + pos, data.begin() + end));
            break;
        }

        uint32_t box_size = decrypt_utils::read_uint_32_big_end(data, pos);
        if (box_size < 8 || box_size > end - pos)
        {
            parts.push_back(vector(data.begin() + pos, data.begin() + end));
            break;
        }

        string type(data.begin() + pos + 4, data.begin() + pos + 8);

        // # 忽略加密相关的 box
        if (skip_boxes.contains(type))
        {
            pos += box_size;
            continue;
        }
        // enca (Encrypted Audio) -> mp4a (Plain Audio)
        // 注意：这里我们盲目的把 enca 改成 mp4a。
        // 如果原始流是 FLAC，这在 rebuild mp4 时可能不够完美，但我们有新的提取逻辑处理 FLAC 提取。
        // 若用户选择下载 .mp4 (非FLAC)，这个逻辑能保证基本的播放兼容性。
        // enca -> mp4a
        if (type == "enca")
        {
            vector<uint8_t> inner = process_box_tree(data, pos, box_size, new_mdat_offset);
            vector<uint8_t> tmp = decrypt_utils::write_uint_32_big_end(static_cast<uint32_t>(inner.size() + 8));
            tmp.insert(tmp.end(), {'m', 'p', '4', 'a'});
            tmp.insert(tmp.end(), inner.begin(), inner.end());
            parts.push_back(tmp);
            pos += box_size;
            continue;
        }

        // # stco -> 更新 chunk offsets
        if (type == "stco")
        {
            vector<uint32_t> new_offsets = decrypt_utils::calculate_chunk_offsets(g_sample_sizes, g_stsc_entries, g_chunk_count, new_mdat_offset);
            vector<uint8_t> new_body = decrypt_utils::update_stco(vector(data.begin() + pos + 8, data.begin() + pos + box_size), new_offsets);
            vector<uint8_t> tmp;
            vector<uint8_t> size_bytes = decrypt_utils::write_uint_32_big_end(static_cast<uint32_t>(new_body.size() + 8));
            tmp.insert(tmp.end(), size_bytes.begin(), size_bytes.end());
            tmp.insert(tmp.end(), {'s', 't', 'c', 'o'});
            tmp.insert(tmp.end(), new_body.begin(), new_body.end());
            parts.push_back(tmp);
            pos += box_size;
            continue;
        }

        // # 容器递归
        if (container_boxes.contains(type))
        {
            vector<uint8_t> inner = process_box_tree(data, pos, box_size, new_mdat_offset);
            vector<uint8_t> tmp;
            vector<uint8_t> size_bytes = decrypt_utils::write_uint_32_big_end(static_cast<uint32_t>(inner.size() + 8));
            tmp.insert(tmp.end(), size_bytes.begin(), size_bytes.end());
            tmp.insert(tmp.end(), type.begin(), type.end());
            tmp.insert(tmp.end(), inner.begin(), inner.end());
            parts.push_back(tmp);
            pos += box_size;
            continue;
        }

        // # 其他 box 原样
        parts.push_back(vector(data.begin() + pos, data.begin() + pos + box_size));
        pos += box_size;
    }

    return decrypt_utils::concat_uint_8_arrays(parts);
}
