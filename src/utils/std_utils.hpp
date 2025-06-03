#pragma once

#include <algorithm>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <cmath>

namespace std
{
    template <size_t N>
    struct string_literal
    {
        constexpr string_literal(const char (&str)[N])
        {
            copy_n(str, N, value);
        }

        char value[N];
    };

    struct string_hash
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char* txt) const noexcept
        {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(std::string_view txt) const noexcept
        {
            return std::hash<std::string_view>{}(txt);
        }
        [[nodiscard]] size_t operator()(const std::string& txt) const noexcept
        {
            return std::hash<std::string>{}(txt);
        }
    };

    inline uint64_t from_hex(std::string_view str, bool* success = nullptr)
    {
        static constexpr uint8_t lut[103] =
            {// '0'-'9'
             0,
             1,
             2,
             3,
             4,
             5,
             6,
             7,
             8,
             9,
             // ':' - '@' (invalid)
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             // 'A'-'F'
             10,
             11,
             12,
             13,
             14,
             15,
             // 'G'-'`' (invalid)
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             0xFF,
             // 'a'-'f'
             10,
             11,
             12,
             13,
             14,
             15};

        uint64_t result = 0;
        if (success)
            *success = true;

        for (char c : str)
        {
            uint8_t index = static_cast<uint8_t>(c) - '0';
            if (index >= sizeof(lut) || lut[index] == 0xFF)
            {
                if (success)
                    *success = false;
                break;
            }

            result = (result << 4) | lut[index];
        }

        return result;
    }

    inline std::string to_hex(uint64_t val)
    {
        static constexpr char lut[] = "0123456789ABCDEF";
        char                  buf[16]; // Max 16 hex digits for uint64_t
        int                   i = 15;

        if (val == 0)
        {
            return "0";
        }

        while (val > 0 && i >= 0)
        {
            buf[i--] = lut[val & 0xF];
            val >>= 4;
        }

        return std::string(&buf[i + 1], &buf[16]);
    }

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
        if (hex.size() < 9 || hex[0] != '#')
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
        string     result      = "#00000000"; // Preallocate string

        for (int i = 7; i >= 0; --i)
        {
            result[1 + i] = hexDigits[value & 0xF]; // Extract last hex digit
            value >>= 4;                            // Shift right by 4 bits (one hex digit)
        }

        return result;
    }

    inline optional<int> view_to_int(string_view str)
    {
        int value      = 0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

        if (ec == errc())
        {
            return value; // Successful conversion
        }
        return nullopt; // Return empty optional if conversion fails
    }

    inline optional<float> view_to_float(string_view str)
    {
        float value    = 0;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

        if (ec == errc())
        {
            return value; // Successful conversion
        }
        return nullopt; // Return empty optional if conversion fails
    }

    inline uint64_t generate_unique_id()
    {
        using namespace std::chrono;

        static std::atomic<uint32_t> counter{0};

        uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

        uint32_t count = counter.fetch_add(1, std::memory_order_relaxed);

        return (now << 16) | (count & 0xFFFF);
    }

    inline string_view trim_left(string_view sv)
    {
        size_t i = 0;
        while (i < sv.size() && isspace(sv[i]))
            ++i;
        return sv.substr(i);
    }

    inline string_view trim_right(string_view sv)
    {
        size_t i = sv.size();
        while (i > 0 && isspace(sv[i - 1]))
            --i;
        return sv.substr(0, i);
    }

    inline string_view trim(string_view sv)
    {
        return trim_right(trim_left(sv));
    }

} // namespace std
