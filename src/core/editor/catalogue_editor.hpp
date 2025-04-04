#pragma once

#include "core/prototype.hpp"
#include "core/atlas_scene_object.hpp"
#include "file_editor.hpp"
#include "json_editor.hpp"

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

    msg::Var _data;

    JsonEdit _edit;
};

inline bool CatalogueFileEdit::on_init(std::string_view path)
{
    auto *sch = R"(
    {
        "type" : "object",
        "properties" :
        {
            "id" : { "type" : "integer", "minimum" : 0, "description": "Test value for int var" },
            "name" :  { "type" : "string", "title" : "Name" },
            "sprite" :  { "type" : "sprite",  "title" : "Sprite" },
            "value" :  { "type" : "number", "minimum" : 0.0 },
            "visible" :  { "type" : "boolean", "default":true },
            "color" :  { "type" : "color" },
            "street": { "enum": ["Street", "Avenue", "Boulevard"] },
            "obj" :
            {
                "type" : "object",
                "title" : "Object",
                "properties" :
                {
                    "id" : { "type" : "integer" },
                    "name" :  { "type" : "string" },
                    "item" : { "$ref" : "#/$def/item_t" }
                },
                "description": "Test value for int var"
            },
            "tags": {
                  "type": "array",
                  "items": {
                    "type": "string"
                  }
                }
        },
        "$def" :
        {
            "item_t" :
             {
                 "type" : "object",
                 "title" : "Object",
                 "properties" :
                 {
                     "id" : { "type" : "integer" },
                     "name" :  { "type" : "string" }
                 },
                 "description": "Test value for int var"
             }
        }
    }
        )";

    _cat = Catalogue::load_shared(path);
    _edit.schema(sch);

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
    _edit.show(_data);

    return false;
}

} // namespace fin
