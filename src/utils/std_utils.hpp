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

} // namespace std
