#pragma once

#include <array>
#include <vector>
#include <string>

inline constexpr std::string_view BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr std::array<uint8_t, 256> makeBase64Rev() {
    std::array<uint8_t, 256> t{};
    t.fill(0xFF);
    for (uint8_t i = 0; i < 64; i++)
        t[(uint8_t)BASE64_CHARS[i]] = i;
    return t;
}

inline constexpr std::array<uint8_t, 256> BASE64_REV = makeBase64Rev();

std::string base64Encode(const std::vector<uint8_t> &data);

std::vector<uint8_t> base64Decode(const std::string &in);