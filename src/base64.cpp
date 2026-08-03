#include "base64.hpp"

std::string base64Encode(const std::vector<uint8_t> &data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    for (size_t i = 0; i < data.size(); i += 3)
    {
        uint32_t b = data[i] << 16;
        if (i + 1 < data.size())
            b |= data[i + 1] << 8;
        if (i + 2 < data.size())
            b |= data[i + 2];

        out += BASE64_CHARS[(b >> 18) & 0x3F];
        out += BASE64_CHARS[(b >> 12) & 0x3F];
        out += (i + 1 < data.size()) ? BASE64_CHARS[(b >> 6) & 0x3F] : '=';
        out += (i + 2 < data.size()) ? BASE64_CHARS[(b >> 0) & 0x3F] : '=';
    }
    return out;
}

std::vector<uint8_t> base64Decode(const std::string &in) {
    std::vector<uint8_t> out;
    out.reserve((in.size() / 4) * 3);

    for (size_t i = 0; i < in.size(); i += 4)
    {
        uint32_t b = (BASE64_REV[(uint8_t)in[i]] << 18)
        | (BASE64_REV[(uint8_t)in[i + 1]] << 12)
        | (BASE64_REV[(uint8_t)in[i + 2]] << 6) // might be '='
        | (BASE64_REV[(uint8_t)in[i + 3]]);     // might be '='

        out.push_back((b >> 16) & 0xFF);
        if (in[i + 2] != '=')
            out.push_back((b >> 8) & 0xFF);
        if (in[i + 3] != '=')
            out.push_back(b & 0xFF);
    }
    return out;
}