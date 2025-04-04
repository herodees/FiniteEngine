#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <queue>
#include <memory>
#include <cmath>
#include <mutex>
#include <algorithm>
#include <functional>

namespace std
{

struct string_hash
{
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const char *txt) const
    {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const
    {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(const std::string &txt) const
    {
        return std::hash<std::string>{}(txt);
    }
};

inline uint32_t hex_char_to_uint(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0; // Invalid character (should handle errors in real cases)
}

inline uint32_t hex_to_color(string_view hex, uint32_t def = 0)
{
    if (hex.size() < 9 || hex [0] != '#')
        return def;

    uint32_t result = 0;

    for (int i = 1; i <= 8; i++)
    {
        result = (result << 4) | hex_char_to_uint(hex[i]);
    }

    return result;
}

inline string color_to_hex(uint32_t value)
{
    const char hexDigits[] = "0123456789ABCDEF";
    string result = "#00000000"; // Preallocate string

    for (int i = 7; i >= 0; --i)
    {
        result[1 + i] = hexDigits[value & 0xF]; // Extract last hex digit
        value >>= 4;                            // Shift right by 4 bits (one hex digit)
    }

    return result;
}

inline optional<int> view_to_int(string_view str)
{
    int value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec == errc())
    {
        return value; // Successful conversion
    }
    return nullopt; // Return empty optional if conversion fails
}

inline optional<float> view_to_float(string_view str)
{
    float value = 0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec == errc())
    {
        return value; // Successful conversion
    }
    return nullopt; // Return empty optional if conversion fails
}

} // namespace std
