//
// Created by baizeyv on 2/1/2026.
//

#ifndef SODADOWNLOADER_DECRYPTOR_H
#define SODADOWNLOADER_DECRYPTOR_H
#include <string>
#include <vector>

#include "mp4_box.h"
#include "parser.h"
#include "StscEntry.h"

using namespace std;

class decryptor
{
    static vector<uint32_t> sampleSizes;

    static vector<StscEntry> stscEntries;

    static uint32_t chunkCount;

    static int bitCount(int n);

    static uint8_t decodeBase36(uint8_t c);

    static vector<uint8_t> decryptSpadeInner(const vector<uint8_t>& spadeKey);

    static string decryptSpade(const vector<uint8_t>& spadeKeyBytes);

    static string decryptSpadeA(const string& spadeA);

    static vector<uint8_t> aesCtrDecrypt(const vector<uint8_t>& key, const vector<uint8_t>& iv, const vector<uint8_t>& data);

    static vector<MP4Box> findBoxes(const vector<uint8_t>& data, const string& type);

    static vector<uint32_t> parseStsz(const vector<uint8_t>& data);

    static std::vector<StscEntry> parseStsc(const std::vector<uint8_t>& data);

    static vector<vector<uint8_t>> parseSenc(const vector<uint8_t>& data);

    static std::vector<uint8_t> scanForFlacMetadata(const std::vector<uint8_t>& stsdData);

    static std::vector<uint32_t> calculateChunkOffsets(
        const std::vector<uint32_t>& sampleSizes,
        const std::vector<StscEntry>& stsc,
        uint32_t chunkCount,
        uint32_t baseOffset
    );

    static std::vector<uint8_t> updateStco(const std::vector<uint8_t>& data, const std::vector<uint32_t>& offsets);

    static std::vector<uint8_t> processBoxTree(const std::vector<uint8_t>& data, size_t offset, size_t size, uint32_t newMdatOffset);

    static std::vector<uint8_t> buildBox(const std::vector<uint8_t>& boxData, const std::string& type);
    
public:
    static void decrypt(const string& spade_a, const string& output_path, const parser& ps);
};


#endif //SODADOWNLOADER_DECRYPTOR_H
