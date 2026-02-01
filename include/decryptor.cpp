//
// Created by baizeyv on 2/1/2026.
//

#include "decryptor.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <openssl/evp.h>

#include "utils.h"

vector<uint32_t> decryptor::sampleSizes{};
vector<StscEntry> decryptor::stscEntries{};
uint32_t decryptor::chunkCount = 0;

int decryptor::bitCount(int n)
{
    n = n - ((n >> 1) & 0x55555555);
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
    return (((n + (n >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
}

uint8_t decryptor::decodeBase36(uint8_t c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return 0xFF;
}

vector<uint8_t> decryptor::decryptSpadeInner(const vector<uint8_t>& spadeKey)
{
    vector<uint8_t> result = spadeKey;
    vector<uint8_t> buff(2 + spadeKey.size());
    buff[0] = 0xFA;
    buff[1] = 0x55;
    copy(spadeKey.begin(), spadeKey.end(), buff.begin() + 2);
    for (size_t i = 0; i < result.size(); ++i)
    {
        int v = (spadeKey[i] ^ buff[i]) - bitCount((int)i) - 21;
        while (v < 0) v += 0xFF;
        result[i] = (uint8_t)v;
    }
    return result;
}

std::string decryptor::decryptSpade(const vector<uint8_t>& spadeKeyBytes)
{
    if (spadeKeyBytes.size() < 3) return "";
    int paddingLen = (spadeKeyBytes[0] ^ spadeKeyBytes[1] ^ spadeKeyBytes[2]) - 48;
    if (static_cast<int>(spadeKeyBytes.size()) < paddingLen + 2) return "";
    vector<uint8_t> innerInput(spadeKeyBytes.begin() + 1, spadeKeyBytes.end() - paddingLen);
    auto tmpBuff = decryptSpadeInner(innerInput);
    if (tmpBuff.empty()) return "";
    size_t skipBytes = decodeBase36(tmpBuff[0]);
    size_t decodedMessageLen = spadeKeyBytes.size() - paddingLen - 2;
    size_t endIndex = 1 + decodedMessageLen - skipBytes;
    if (endIndex > tmpBuff.size()) return "";
    vector<uint8_t> finalBytes(tmpBuff.begin() + 1, tmpBuff.begin() + endIndex);
    return string(finalBytes.begin(), finalBytes.end());
}

string decryptor::decryptSpadeA(const string& spadeA)
{
    string decoded;
    try
    {
        string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        string binary;
        int val = 0, valb = -8;
        for (char c : spadeA)
        {
            if (isspace(c)) continue;
            if (c == '=') break; // 遇到 = 就结束解码，不加入 0
            int idx = base64_chars.find(c);
            if (idx == string::npos) throw runtime_error("非法 base64字符");
            val = (val << 6) + idx;
            valb += 6;
            if (valb >= 0)
            {
                binary.push_back((val >> valb) & 0xFF);
                valb -= 8;
            }
        }
        vector<uint8_t> bytes(binary.begin(), binary.end());
        return decryptSpade(bytes);
    }
    catch (...)
    {
        return "";
    }
}

vector<uint8_t> decryptor::aesCtrDecrypt(const vector<uint8_t>& key, const vector<uint8_t>& iv, const vector<uint8_t>& data)
{
    vector<uint8_t> out(data.size());
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, key.data(), iv.data());
    int len = 0;
    EVP_DecryptUpdate(ctx, out.data(), &len, data.data(), (int)data.size());
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

vector<MP4Box> decryptor::findBoxes(const vector<uint8_t>& data, const string& type)
{
    vector<MP4Box> boxes;
    size_t pos = 0;
    while (pos + 8 <= data.size())
    {
        uint32_t size = utils::readUint32BE(data, pos);
        string t((char*)&data[pos + 4], (char*)&data[pos + 8]);
        if (t == type) boxes.push_back({size, t, pos, {}});
        if (size < 8) break;
        pos += size;
    }
    return boxes;
}

vector<uint32_t> decryptor::parseStsz(const vector<uint8_t>& data)
{
    uint32_t sampleSize = utils::readUint32BE(data, 4);
    uint32_t count = utils::readUint32BE(data, 8);
    vector<uint32_t> sizes;
    if (sampleSize != 0)
    {
        sizes.assign(count, sampleSize);
    }
    else
    {
        for (uint32_t i = 0; i < count; ++i) sizes.push_back(utils::readUint32BE(data, 12 + i * 4));
    }
    return sizes;
}

std::vector<StscEntry> decryptor::parseStsc(const std::vector<uint8_t>& data)
{
    uint32_t entryCount = utils::readUint32BE(data, 4);
    std::vector<StscEntry> entries;
    entries.reserve(entryCount);

    for (uint32_t i = 0; i < entryCount; ++i)
    {
        size_t b = 8 + i * 12;
        StscEntry e;
        e.firstChunk = utils::readUint32BE(data, b);
        e.samplesPerChunk = utils::readUint32BE(data, b + 4);
        e.id = utils::readUint32BE(data, b + 8);
        entries.push_back(e);
    }
    return entries;
}

vector<vector<uint8_t>> decryptor::parseSenc(const vector<uint8_t>& data)
{
    uint32_t count = utils::readUint32BE(data, 4);
    vector<vector<uint8_t>> ivs;
    size_t pos = 8;
    for (uint32_t i = 0; i < count; i++)
    {
        vector<uint8_t> iv(16, 0);
        copy(data.begin() + pos, data.begin() + pos + 8, iv.begin());
        ivs.push_back(iv);
        pos += 8;
    }
    return ivs;
}

std::vector<uint8_t> decryptor::scanForFlacMetadata(const std::vector<uint8_t>& stsdData)
{
    const uint8_t searchStr[4] = {0x64, 0x66, 0x4C, 0x61}; // "dfLa"

    for (size_t i = 0; i + 4 <= stsdData.size(); ++i)
    {
        if (stsdData[i] == searchStr[0] &&
            stsdData[i + 1] == searchStr[1] &&
            stsdData[i + 2] == searchStr[2] &&
            stsdData[i + 3] == searchStr[3])
        {
            if (i < 4) throw std::runtime_error("dfLa 前面不足4字节读取 boxSize");

            uint32_t boxSize = utils::readUint32BE(stsdData, i - 4);

            // 内容部分：跳过 size(4B) + type(4B)，只返回 box 内部数据
            size_t contentStart = i + 4;
            size_t contentEnd = (i - 4 + boxSize > stsdData.size()) ? stsdData.size() : i - 4 + boxSize;

            if (contentEnd <= contentStart) return {}; // 防空
            return std::vector<uint8_t>(stsdData.begin() + contentStart, stsdData.begin() + contentEnd);
        }
    }
    return {}; // 没找到
}

std::vector<uint32_t> decryptor::calculateChunkOffsets(const std::vector<uint32_t>& sampleSizes, const std::vector<StscEntry>& stsc,
                                                       uint32_t chunkCount, uint32_t baseOffset)
{
    std::vector<uint32_t> offsets;
    offsets.reserve(chunkCount);

    uint32_t current = baseOffset;
    size_t sIdx = 0;

    for (uint32_t c = 1; c <= chunkCount; ++c)
    {
        offsets.push_back(current);

        // 找到当前 chunk 的 samplesPerChunk
        uint32_t count = 0;
        for (size_t i = 0; i < stsc.size(); ++i)
        {
            bool isLast = (i + 1 >= stsc.size());
            if (c >= stsc[i].firstChunk && (isLast || c < stsc[i + 1].firstChunk))
            {
                count = stsc[i].samplesPerChunk;
                break;
            }
        }

        // 累加 sampleSizes
        for (uint32_t k = 0; k < count && sIdx < sampleSizes.size(); ++k)
        {
            current += sampleSizes[sIdx++];
        }
    }

    return offsets;
}

std::vector<uint8_t> decryptor::updateStco(const std::vector<uint8_t>& data, const std::vector<uint32_t>& offsets)
{
    // 取前8字节作为 header
    std::vector<uint8_t> header(data.begin(), data.begin() + 8);

    // 构建 body
    std::vector<uint8_t> body(offsets.size() * 4);
    for (size_t i = 0; i < offsets.size(); ++i)
    {
        utils::writeUint32BE(&body[i * 4], offsets[i]);
    }

    // 拼接 header + body
    return utils::concatUint8Arrays({header, body});
}

std::vector<uint8_t> decryptor::processBoxTree(const std::vector<uint8_t>& data, size_t offset, size_t size, uint32_t newMdatOffset)
{
    std::vector<std::vector<uint8_t>> parts;
    size_t pos = offset + 8; // 跳过 box header
    size_t end = offset + size;

    unordered_set<std::string> skipBoxes = {"senc", "saio", "saiz", "sinf", "schi", "tenc", "schm", "frma"};
    unordered_set<std::string> containerBoxes = {"moov", "trak", "mdia", "minf", "stbl", "stsd"};

    while (pos < end)
    {
        if (pos + 8 > end)
        {
            parts.push_back(std::vector<uint8_t>(data.begin() + pos, data.begin() + end));
            break;
        }

        uint32_t boxSize = utils::readUint32BE(data, pos);
        if (boxSize < 8 || boxSize > end - pos)
        {
            parts.push_back(std::vector<uint8_t>(data.begin() + pos, data.begin() + end));
            break;
        }

        std::string type(data.begin() + pos + 4, data.begin() + pos + 8);

        // 忽略加密相关 box
        if (skipBoxes.count(type))
        {
            pos += boxSize;
            continue;
        }
        // enca (Encrypted Audio) -> mp4a (Plain Audio)
        // 注意：这里我们盲目的把 enca 改成 mp4a。
        // 如果原始流是 FLAC，这在 rebuild mp4 时可能不够完美，但我们有新的提取逻辑处理 FLAC 提取。
        // 若用户选择下载 .mp4 (非FLAC)，这个逻辑能保证基本的播放兼容性。
        // enca -> mp4a
        if (type == "enca")
        {
            std::vector<uint8_t> inner = processBoxTree(data, pos, boxSize, newMdatOffset);
            std::vector<uint8_t> tmp = utils::writeUint32BE(static_cast<uint32_t>(inner.size() + 8));
            tmp.insert(tmp.end(), {'m', 'p', '4', 'a'});
            tmp.insert(tmp.end(), inner.begin(), inner.end());
            parts.push_back(tmp);
            pos += boxSize;
            continue;
        }

        // stco -> 更新 chunk offsets
        if (type == "stco")
        {
            std::vector<uint32_t> newOffsets = calculateChunkOffsets(
                sampleSizes, stscEntries, chunkCount,
                newMdatOffset
            );
            std::vector<uint8_t> newBody = updateStco(std::vector<uint8_t>(data.begin() + pos + 8, data.begin() + pos + boxSize), newOffsets);
            std::vector<uint8_t> tmp;
            std::vector<uint8_t> sizeBytes = utils::writeUint32BE(static_cast<uint32_t>(newBody.size() + 8));
            tmp.insert(tmp.end(), sizeBytes.begin(), sizeBytes.end());
            tmp.insert(tmp.end(), {'s', 't', 'c', 'o'});
            tmp.insert(tmp.end(), newBody.begin(), newBody.end());
            parts.push_back(tmp);
            pos += boxSize;
            continue;
        }

        // 容器 box 递归
        if (containerBoxes.count(type))
        {
            std::vector<uint8_t> inner = processBoxTree(data, pos, boxSize, newMdatOffset);
            std::vector<uint8_t> tmp;
            std::vector<uint8_t> sizeBytes = utils::writeUint32BE(static_cast<uint32_t>(inner.size() + 8));
            tmp.insert(tmp.end(), sizeBytes.begin(), sizeBytes.end());
            tmp.insert(tmp.end(), type.begin(), type.end());
            tmp.insert(tmp.end(), inner.begin(), inner.end());
            parts.push_back(tmp);
            pos += boxSize;
            continue;
        }

        // 其他 box 原样
        parts.push_back(std::vector<uint8_t>(data.begin() + pos, data.begin() + pos + boxSize));
        pos += boxSize;
    }

    return utils::concatUint8Arrays(parts);
}

std::vector<uint8_t> decryptor::buildBox(const std::vector<uint8_t>& boxData, const std::string& type)
{
    std::vector<uint8_t> result;

    // 1. box 长度 = 数据长度 + 8
    uint32_t boxSize = static_cast<uint32_t>(boxData.size() + 8);
    auto sizeBytes = utils::writeUint32BE(boxSize);
    result.insert(result.end(), sizeBytes.begin(), sizeBytes.end());

    // 2. box 类型
    result.insert(result.end(), type.begin(), type.end()); // type 必须是 4 字节

    // 3. box 内容
    result.insert(result.end(), boxData.begin(), boxData.end());

    return result;
}

void decryptor::decrypt(const string& spade_a, const string& output_path, const parser& ps)
{
    const auto root = utils::get_save_root().string();
    string filePath = root + "/download.tmp";

    vector<uint8_t> fileData = utils::readFile(filePath);

    string keyHex = spade_a;
    if (spade_a.find_first_not_of("0123456789abcdefABCDEF") != string::npos)
    {
        keyHex = decryptSpadeA(spade_a);
        if (keyHex.empty())
        {
            cerr << "Spade 解密失败";
            return;
        }
    }
    cout << "[*] Hex Key: " << keyHex << endl;
    vector<uint8_t> keyBytes = utils::hexToBytes(keyHex);

    auto ftyp = MP4Box::findBox(fileData, "ftyp");
    auto moov = MP4Box::findBox(fileData, "moov");
    if (moov.empty())
    {
        cerr << "未找到 moov atom" << endl;
        return;
    }

    auto trak = MP4Box::findBox(fileData, "trak", moov.offset + 8, moov.offset + moov.size);
    auto mdia = MP4Box::findBox(fileData, "mdia", trak.offset + 8, trak.offset + trak.size);
    auto minf = MP4Box::findBox(fileData, "minf", mdia.offset + 8, mdia.offset + mdia.size);
    auto stbl = MP4Box::findBox(fileData, "stbl", minf.offset + 8, minf.offset + moov.size);
    // ! 关键：获取 stsd 以检查是否为 FLAC
    auto stsd = MP4Box::findBox(fileData, "stsd", stbl.offset + 8, stbl.offset + stbl.size);

    auto flacMetadata = scanForFlacMetadata(stsd.data);
    bool isFlac = !flacMetadata.empty();

    if (isFlac)
        cout << "[!] 检测到 FLAC 编码！将提取为 .flac 文件" << endl;
    else
        cout << "[*] 未检测到 FLAC (可能是 AAC/AVC)，将重建为 MP4/M4A" << endl;

    auto stsz = MP4Box::findBox(fileData, "stsz", stbl.offset + 8, stbl.offset + stbl.size);
    auto stsc = MP4Box::findBox(fileData, "stsc", stbl.offset + 8, stbl.offset + stbl.size);
    auto stco = MP4Box::findBox(fileData, "stco", stbl.offset + 8, stbl.offset + stbl.size);

    sampleSizes = parseStsz(stsz.data);
    stscEntries = parseStsc(stsc.data);
    chunkCount = utils::readUint32BE(stco.data, 4);

    auto senc = MP4Box::findBox(fileData, "senc", stbl.offset + 8, stbl.offset + stbl.size);
    if (senc.empty())
        senc = MP4Box::findBox(fileData, "senc", moov.offset + 8, moov.offset + moov.size);
    if (senc.empty())
    {
        cerr << "未找到加密信息 (senc)" << endl;
        return ;
    }

    auto ivs = parseSenc(senc.data);
    auto mdat = MP4Box::findBox(fileData, "mdat");

    cout << "[*] 样本数: " << sampleSizes.size() << ", IV数: " << ivs.size() << endl;

    cout << "[*] 开始 AES-CTR 解密..." << endl;

    // AES-CTR 解密
    vector<vector<uint8_t>> decryptedSamples;
    size_t sampleOffset = mdat.offset + 8;
    for (size_t i = 0; i < sampleSizes.size(); i++)
    {
        size_t size = sampleSizes[i];
        auto iv = ivs[i];

        vector<uint8_t> encrypted(fileData.begin() + sampleOffset, fileData.begin() + sampleOffset + size);
        vector<uint8_t> dec = aesCtrDecrypt(keyBytes, iv, encrypted);
        decryptedSamples.push_back(dec);
        sampleOffset += size;
        if (i % 100 == 0) cout << "[*] 解密样本 " << i << "/" << sampleSizes.size() << "\r";
    }

    vector<uint8_t> finalData;
    string finalExt;

    if (isFlac)
    {
        // === FLAC 输出 ===
        std::vector<uint8_t> flacSig = {0x66, 0x4C, 0x61, 0x43}; // "fLaC"
        // Metadata body: 去掉前 4 字节 Version+Flags
        std::vector<uint8_t> metaBody;
        if (flacMetadata.size() > 4)
            metaBody.insert(metaBody.end(), flacMetadata.begin() + 4, flacMetadata.end());
        else
            metaBody = flacMetadata;
        std::vector<std::vector<uint8_t>> parts;
        parts.push_back(flacSig);
        parts.push_back(metaBody);
        for (const auto& sample : decryptedSamples)
            parts.push_back(sample);
        finalData = utils::concatUint8Arrays(parts);
        finalExt = ".flac";

        cout << "[*] 已封装为 FLAC (Raw Extraction)" << endl;
    }
    else
    {
        cout << "[*] 重建 MP4 容器..." << endl;
        auto ftypSize = ftyp.empty() ? 0 : ftyp.size;
        auto dummyMoov = processBoxTree(fileData, moov.offset, moov.size, 0);
        uint32_t newMdatOffset = ftypSize + dummyMoov.size() + 8 + 8;

        auto cleanMoovData = processBoxTree(fileData, moov.offset, moov.size, newMdatOffset);
        auto cleanMoov = buildBox(cleanMoovData, "moov");
        auto mDatData = utils::concatUint8Arrays(decryptedSamples);
        auto newMdat = buildBox(mDatData, "mdat");

        if (!ftyp.empty())
        {
            fileData.insert(fileData.end(), fileData.begin() + ftyp.offset, fileData.begin() + ftyp.offset + ftyp.size);
        }
        fileData.insert(fileData.end(), cleanMoov.begin(), cleanMoov.end());
        fileData.insert(fileData.end(), newMdat.begin(), newMdat.end());
        finalExt = ".m4a"; // ? 这里也可以是mp4
    }
    cout << "[*] 完成! 输出大小: " + to_string(fileData.size() / 1024 / 1024) + " MB";

    // # 输出文件
    string outFile = output_path + "/" + ps.get_track_title() + " - " + ps.get_track_artist() + finalExt;
    utils::writeFile(outFile, finalData);
    cout << "[*] 完成 输出: " << outFile << endl;
}
