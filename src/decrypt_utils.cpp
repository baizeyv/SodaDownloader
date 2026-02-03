//
// Created by baizeyv on 2/3/2026.
//

#include "decrypt_utils.h"
#include <openssl/evp.h>

#include <stdexcept>
#include <unordered_set>

int decrypt_utils::bit_count(int n)
{
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    return (((n + (n >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

uint8_t decrypt_utils::decode_base_36(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return 0xFF;
}

vector<uint8_t> decrypt_utils::decrypt_spade_inner(const vector<uint8_t>& spade_key)
{
    vector<uint8_t> result = spade_key;
    vector<uint8_t> buff(2 + spade_key.size());
    buff[0] = 0xFA;
    buff[1] = 0x55;
    ranges::copy(spade_key, buff.begin() + 2);
    for (size_t i = 0; i < result.size(); ++i)
    {
        int v = (spade_key[i] ^ buff[i]) - bit_count(static_cast<int>(i)) - 21;
        while (v < 0)
            v += 0xFF;
        result[i] = static_cast<uint8_t>(v);
    }
    return result;
}

string decrypt_utils::decrypt_spade(const vector<uint8_t>& spade_key_bytes)
{
    if (spade_key_bytes.size() < 3)
        return "";
    const int padding_len = (spade_key_bytes[0] ^ spade_key_bytes[1] ^ spade_key_bytes[2]) - 48;
    if (static_cast<int>(spade_key_bytes.size()) < padding_len + 2)
        return "";
    const vector inner_input(spade_key_bytes.begin() + 1, spade_key_bytes.end() - padding_len);
    auto tmp_buff = decrypt_spade_inner(inner_input);
    if (tmp_buff.empty())
        return "";
    const size_t skip_bytes = decode_base_36(tmp_buff[0]);
    const size_t decoded_message_len = spade_key_bytes.size() - padding_len - 2;
    const size_t end_index = 1 + decoded_message_len - skip_bytes;
    if (end_index > tmp_buff.size())
        return "";
    vector final_bytes(tmp_buff.begin() + 1, tmp_buff.begin() + end_index);
    return string(final_bytes.begin(), final_bytes.end());
}

string decrypt_utils::decrypt_spade_a(const string& spade_a)
{
    string decoded;
    try
    {
        string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        string binary;

        int val = 0, val_b = -8;

        for (const char c : spade_a)
        {
            if (isspace(c))
                continue;
            if (c == '=') // # 遇到 = 就结束解码, 不加入0
                break;
            const int idx = base64_chars.find(c);
            if (idx == string::npos)
                throw runtime_error("illegal base64 character.");
            val = (val << 6) + idx;
            val_b += 6;
            if (val_b >= 0)
            {
                binary.push_back((val >> val_b) & 0xFF);
                val_b -= 8;
            }
        }

        const vector<uint8_t> bytes(binary.begin(), binary.end());
        return decrypt_spade(bytes);
    }
    catch (...)
    {
        return "";
    }
}

vector<uint8_t> decrypt_utils::aes_ctr_decrypt(const vector<uint8_t>& key, const vector<uint8_t>& iv, const vector<uint8_t>& data)
{
    vector<uint8_t> out(data.size());
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key.data(), iv.data());
    int len = 0;
    EVP_DecryptUpdate(ctx, out.data(), &len, data.data(), static_cast<int>(data.size()));
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

vector<uint32_t> decrypt_utils::parse_stsz(const vector<uint8_t>& data)
{
    const uint32_t sample_size = read_uint_32_big_end(data, 4);
    const uint32_t count = read_uint_32_big_end(data, 8);
    vector<uint32_t> sizes;
    if (sample_size != 0)
    {
        sizes.assign(count, sample_size);
    }
    else
    {
        for (uint32_t i = 0; i < count; ++i)
            sizes.push_back(read_uint_32_big_end(data, 12 + i * 4));
    }
    return sizes;
}

vector<stsc_entry> decrypt_utils::parse_stsc(const vector<uint8_t>& data)
{
    const uint32_t entry_count = read_uint_32_big_end(data, 4);
    vector<stsc_entry> entries;
    entries.reserve(entry_count);

    for (uint32_t i = 0; i < entry_count; ++i)
    {
        const size_t b = 8 + i * 12;
        stsc_entry e;
        e.first_chunk = read_uint_32_big_end(data, b);
        e.samples_per_chunk = read_uint_32_big_end(data, b + 4);
        e.id = read_uint_32_big_end(data, b + 8);
        entries.push_back(e);
    }

    return entries;
}

vector<vector<uint8_t>> decrypt_utils::parse_senc(const vector<uint8_t>& data)
{
    const uint32_t count = read_uint_32_big_end(data, 4);
    vector<vector<uint8_t>> ivs;
    size_t pos = 8;

    for (uint32_t i = 0; i < count; ++i)
    {
        vector<uint8_t> iv(16, 0);
        copy_n(data.begin() + pos, 8, iv.begin());
        ivs.push_back(iv);
        pos += 8;
    }

    return ivs;
}

vector<uint8_t> decrypt_utils::update_stco(const vector<uint8_t>& data, const vector<uint32_t>& offsets)
{
    // # 取前8个字节作为 header
    vector header(data.begin(), data.begin() + 8);

    // # 构建 body
    vector<uint8_t> body(offsets.size() * 4);
    for (size_t i = 0; i < offsets.size(); ++i)
    {
        write_uint_32_big_end(&body[i * 4], offsets[i]);
    }

    // # 拼接 header + body
    return concat_uint_8_arrays({header, body});
}

vector<uint8_t> decrypt_utils::scan_for_flac_metadata(const vector<uint8_t>& stsd_data)
{
    constexpr uint8_t search_str[4] = {0x64, 0x66, 0x4C, 0x61}; // # dfLa

    for (size_t i = 0; i + 4 <= stsd_data.size(); ++i)
    {
        if (stsd_data[i] == search_str[0] && stsd_data[i + 1] == search_str[1] && stsd_data[i + 2] == search_str[2] && stsd_data[i + 3] == search_str[
            3])
        {
            if (i < 4)
                throw runtime_error("if the first 4 bytes of 'dfLa' are not read, then read box size.");

            const uint32_t box_size = read_uint_32_big_end(stsd_data, i - 4);

            // # 内容部分: 跳过 size(4B) + type(4B), 只返回 box 内部数据
            const size_t content_start = i + 4;
            const size_t content_end = (i - 4 + box_size > stsd_data.size()) ? stsd_data.size() : i - 4 + box_size;

            if (content_end <= content_start)
                return {};
            return vector(stsd_data.begin() + content_start, stsd_data.begin() + content_end);
        }
    }

    // # 没有找到
    return {};
}

vector<uint32_t> decrypt_utils::calculate_chunk_offsets(const vector<uint32_t>& sample_sizes, const vector<stsc_entry>& stsc, uint32_t chunk_count,
                                                        uint32_t base_offset)
{
    vector<uint32_t> offsets;
    offsets.reserve(chunk_count);

    uint32_t current = base_offset;
    size_t s_idx = 0;

    for (uint32_t c = 1; c <= chunk_count; ++c)
    {
        offsets.push_back(current);

        // # 找到当前 chunk 的 samples_per_chunk
        uint32_t count = 0;
        for (size_t i = 0; i < stsc.size(); ++i)
        {
            const bool is_last = (i + 1 >= stsc.size());
            if (c >= stsc[i].first_chunk && (is_last || c < stsc[i + 1].first_chunk))
            {
                count = stsc[i].samples_per_chunk;
                break;
            }
        }

        // # 累加 sample_sizes
        for (uint32_t k = 0; k < count && s_idx < sample_sizes.size(); ++k)
        {
            current += sample_sizes[s_idx++];
        }
    }

    return offsets;
}

uint32_t decrypt_utils::read_uint_32_big_end(const vector<uint8_t>& data, const size_t offset)
{
    return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3];
}

vector<uint8_t> decrypt_utils::write_uint_32_big_end(uint32_t value)
{
    vector<uint8_t> bytes(4);
    bytes[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    bytes[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[3] = static_cast<uint8_t>(value & 0xFF);
    return bytes;
}

void decrypt_utils::write_uint_32_big_end(uint8_t* buf, uint32_t value)
{
    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    buf[3] = value & 0xFF;
}

vector<uint8_t> decrypt_utils::hex_to_bytes(const string& hex)
{
    vector<uint8_t> bytes;
    if (hex.size() % 2 != 0)
        throw runtime_error("the length of Hex must be even.");
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        uint8_t b = static_cast<uint8_t>(stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

string decrypt_utils::read_box_type(const vector<uint8_t>& data, size_t offset)
{
    if (offset + 8 > data.size())
        throw runtime_error("index of data out of bounds.");
    char type[5] = {0};
    memcpy(type, &data[offset + 4], 4);
    return string(type);
}

vector<uint8_t> decrypt_utils::concat_uint_8_arrays(const vector<vector<uint8_t>>& arrays)
{
    // # 先计算总长度
    size_t total_length = 0;
    for (const auto& arr : arrays)
        total_length += arr.size();

    // # 创建结果 vector
    vector<uint8_t> result;
    result.reserve(total_length); // # 提前分配内存

    // # 拼接所有数组
    for (const auto& arr : arrays)
    {
        result.insert(result.end(), arr.begin(), arr.end());
    }
    return result;
}
