#pragma once

#include "include.hpp"
#include "utils/imgui_utils.hpp"

namespace fin
{
class FileEdit
{
public:
    FileEdit() = default;
    virtual ~FileEdit() = default;

    virtual bool on_init(std::string_view path) = 0;
    virtual bool on_deinit() = 0;
    virtual bool on_edit() = 0;

    static FileEdit *create_editor_from_file(std::string_view filename);
};
}
