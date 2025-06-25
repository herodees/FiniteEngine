#pragma once
#include "svstream.hpp"
#include <vector>
#include <string>
#include <cassert>

namespace std
{
    struct ini_config
    {
        struct section_key
        {
            std::string_view section;
            std::string_view key;
            std::string_view value;
        };

        bool             parse(std::string_view source);
        bool             parse_inplace(std::string_view source);
        void             add_section(std::string_view section);
        void             add_key(std::string_view key, std::string_view value);
        std::string_view get_section(std::string_view section, std::string_view key, std::string_view defvalue = {});
        int              get_section(std::string_view section, std::string_view key, int defvalue);
        float            get_section(std::string_view section, std::string_view key, float defvalue);

        std::string              _buff;
        std::vector<section_key> _sections;
    };


    inline bool ini_config::parse(std::string_view source)
    {
        _buff = source;
        return parse_inplace(_buff);
    }

    inline bool ini_config::parse_inplace(std::string_view source)
    {
        _sections.clear();

        std::string_view current_section = "";
        std::string_view line;

        std::string_view_stream stream(source);

        while (!stream.eof())
        {
            // Grab line
            size_t line_start = stream.remaining().data() - source.data();
            stream.skip_until('\n');
            size_t line_end = stream.remaining().data() - source.data();
            line            = source.substr(line_start, line_end - line_start);

            std::string_view_stream line_stream(line);
            line_stream.skip_whitespace();

            char first = line_stream.peek();

            // Skip blank/comment lines
            if (first == '\0' || first == '#' || first == ';')
                continue;

            // Section header
            if (first == '[')
            {
                line_stream.get(); // skip [
                size_t name_start = line_stream.remaining().data() - source.data();
                line_stream.skip_until(']');
                size_t name_end = line_stream.remaining().data() - source.data() - 1;

                if (name_end > name_start)
                {
                    current_section = source.substr(name_start, name_end - name_start);
                    add_section(current_section);
                }
                continue;
            }

            // Key-value
            auto key = line_stream.parse_identifier();
            line_stream.skip_whitespace();

            if (!line_stream.expect('='))
                continue;

            line_stream.skip_whitespace();
            size_t value_start = line_stream.remaining().data() - source.data();
            size_t value_end   = line.find_first_of("#;\n\r", value_start);
            if (value_end == std::string_view::npos)
                value_end = source.size();

            std::string_view value = source.substr(value_start, value_end - value_start);

            // Trim trailing whitespace
            while (!value.empty() && std::isspace(value.back()))
                value = value.substr(0, value.size() - 1);

            add_key(key, value);
        }

        return !_sections.empty();
    }

    inline void ini_config::add_section(std::string_view section)
    {
        auto& sec = _sections.emplace_back();
        sec.section = section;
    }

    inline void ini_config::add_key(std::string_view key, std::string_view value)
    {
        assert(!_sections.empty() && "Section required!");
        auto& sec = _sections.back();
        if (sec.key.empty())
        {
            sec.key = key;
            sec.value = value;
        }
        else
        {
            _sections.emplace_back(sec.section, key, value);
        }
    }

    inline std::string_view ini_config::get_section(std::string_view section, std::string_view key, std::string_view defvalue)
    {
        for (size_t n = 0; n < _sections.size(); ++n)
        {
            if (_sections[n].section == section)
            {
                for (size_t m = n; m < _sections.size(); ++m)
                {
                    if (_sections[n].section != section)
                        break;
                    if (_sections[n].key == key)
                        return _sections[n].value;
                }
                return defvalue;
            }
        }
        return defvalue;
    }

    inline int ini_config::get_section(std::string_view section, std::string_view key, int defvalue)
    {

        return std::string_view();
    }

    inline float ini_config::get_section(std::string_view section, std::string_view key, float defvalue)
    {
        return 0.0f;
    }
}
