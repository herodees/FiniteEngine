#pragma once

#include "core/prototype.hpp"
#include "core/atlas_scene_object.hpp"
#include "file_editor.hpp"

namespace fin
{

class CatalogueFileEdit : public FileEdit
{
public:
    bool on_init(std::string_view path) override;
    bool on_deinit() override;
    bool on_edit() override;
    bool on_setup() override;

    std::shared_ptr<Catalogue> _cat;
    AtlasSceneObject _object;
};

inline bool CatalogueFileEdit::on_init(std::string_view path)
{
    _cat = Catalogue::load_shared(path);

    return _cat.get();
}

inline bool CatalogueFileEdit::on_deinit()
{
    _cat.reset();
    return false;
}

inline bool CatalogueFileEdit::on_edit()
{
    if (!_cat)
        return false;

    if (ImGui::Selectable(ICON_FA_FOLDER " .."))
    {
        return false;
    }

    return true;
}

inline bool CatalogueFileEdit::on_setup()
{
    return false;
}

} // namespace fin
