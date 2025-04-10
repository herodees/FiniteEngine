#include "file_editor.hpp"
#include "atlas_scene_editor.hpp"
#include "proto_editor.hpp"

namespace fin
{

template <typename T>
static FileEdit *create_editor(std::string_view filename)
{
    auto edit = new T();
    if (edit->on_init(filename))
    {
        return edit;
    }
    delete edit;
    return nullptr;
}

FileEdit *FileEdit::create_editor_from_file(std::string_view filename)
{
    auto ext = path_get_ext(filename);

    if (ext == ".atlas")
        return create_editor<AtlasFileEdit>(filename);

    if (ext == ".proto")
        return create_editor<ProtoFileEdit>(filename);

    return nullptr;
}

} // namespace fin
