
#include "factory.hpp"
#include <core/editor/imgui_control.hpp>
#include <utils/imguiline.hpp>
#include <core/scene.hpp>

namespace fin
{
    namespace
    {
        static constexpr std::string_view MetadataFile = ".metadata";

        struct FileExplorer
        {
            struct IconData
            {
                ImTextureID txt;
                Regionf     uv;
                ImVec2      size;
                const char* label;
                const char* type;
                void*       drag;
                size_t      drag_size;
            };

            std::string                                   _path;
            std::vector<std::string>                      _dirs;
            std::vector<std::string>                      _files;
            std::vector<std::pair<Sprite2D::Ptr, size_t>> _sprites;
            std::shared_ptr<ImGui::Editor>                _editor;
            std::string                                   _metatype;
            std::string_view                              _context;
            size_t                                        _selected{};
            float                                         _icon_size = 56.f;
            float                                         _padding   = 10.0f;
            bool                                          _expanded{};

            void Show(Scene* scene, StringView base_folder);
            void ShowFilesRaw(Scene* scene);
            void ShowFilesMeta(Scene* scene, StringView type);
            void ShowFilesAtlas(Scene* scene);

            void ShowGrid(auto& items);
            void GetItem(const std::pair<Sprite2D::Ptr, size_t>& item, IconData& data);
            void OpenItem(const std::pair<Sprite2D::Ptr, size_t>& item);
        };

        FileExplorer gFileExplorer;
        std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> gAtlasCache;
        std::unordered_map<std::string, Sprite2D::Ptr, std::string_hash, std::equal_to<>> gSpriteCache;

        std::string_view GetFileExt(std::string_view file)
        {
            auto p = file.rfind('.');
            return p == std::string_view::npos ? "" : file.substr(p);
        }

        std::string_view GetFileIcon(std::string_view file, uint32_t& clr)
        {
            clr    = 0xff808080;
            auto p = file.rfind(".");
            if (p == std::string_view::npos)
                return ICON_FA_FILE_PEN;
            auto ext = file.substr(p + 1);
            if (ext == "ogg" || ext == "wav")
            {
                clr = 0xff0051f2;
                return ICON_FA_FILE_AUDIO;
            }
            if (ext == "png" || ext == "jpg" || ext == "gif")
            {
                clr = 0xff60d71e;
                return ICON_FA_FILE_IMAGE;
            }
            if (ext == "atlas")
            {
                clr = 0xffcc6f98;
                return ICON_FA_FILE_ZIPPER;
            }
            if (ext == "prefab")
            {
                clr = 0xfff0f0f0;
                return ICON_FA_FILE_CODE;
            }
            if (ext == "shader")
            {
                clr = 0xffccaf98;
                return ICON_FA_FILE_CODE;
            }
            return ICON_FA_FILE;
        }


        


        class ShaderProperties : public ImGui::Dialog
        {
            Shader2D::Ptr _shader;
            Shader        _sh{};
            std::string   _vs;
            std::string   _fs;
        public:
            ShaderProperties(Shader2D::Ptr spr) :
            _shader(spr),
            _vs(_shader->GetVertexShader()),
            _fs(_shader->GetFragmentShader()) {};
            ~ShaderProperties() final
            {
                if (_sh.id)
                    UnloadShader(_sh);
            }
            bool Compile()
            {
                if (_sh.id)
                    UnloadShader(_sh);
                _sh = LoadShaderFromMemory(_shader->GetVertexShader().c_str(), _shader->GetFragmentShader().c_str());
                return _sh.id != 0;
            }
            bool OnUpdate() final
            {
                bool ret{};
                if (ImGui::BeginTabBar("##TabBar"))
                {
                    if (ImGui::BeginTabItem("Vertex"))
                    {
                        ret |= ImGui::InputTextMultiline("##vs", &_fs, {-1, -ImGui::GetFrameHeightWithSpacing()});
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Fragment"))
                    {
                        ret |= ImGui::InputTextMultiline("##fs", &_vs, {-1, -ImGui::GetFrameHeightWithSpacing()});
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }

                if (ImGui::LineItem("ftr", {-1, ImGui::GetFrameHeight()})
                        .PushStyle(ImStyle_Button, -1)
                        .Text("  Compile  ")
                        .PopStyle()
                        .Spring()
                        .PushStyle(ImStyle_Button, 1)
                        .Text("  Cancel  ")
                        .PopStyle()
                        .Space()
                        .PushStyle(ImStyle_Header, 2)
                        .Text("   Save   ")
                        .PopStyle()
                        .End())
                {
                    if (ImGui::Line().HoverId() == -1)
                    {
                        Compile();
                    }
                    if (ImGui::Line().HoverId() == 1)
                        _shader.reset();
                    if (ImGui::Line().HoverId() == 2 && Compile())
                    {
                        _shader->GetFragmentShader() = _fs;
                        _shader->GetVertexShader() = _vs;
                        if (_shader->SaveToFile(_shader->GetPath()))
                        {
                            _shader.reset();
                        }
                    }
                }

                return !!_shader;
            };
        };



        class SpriteProperties : public ImGui::Dialog
        {
            bool                _update = true;
            Sprite2D::Ptr       _sprite;
            ImGui::CanvasParams _canvas;

        public:
            SpriteProperties(Sprite2D::Ptr spr) : _sprite(spr) {};
            bool OnUpdate() final
            {
                if (ImGui::BeginCanvas("Sprite Properties", {-1, -ImGui::GetFrameHeightWithSpacing()}, _canvas))
                {
                    if (_canvas.canvas_size.x > 0 && _update)
                    {
                        _canvas.snap_grid = {1, 1};
                        _update = false;
                        _canvas.CenterOnScreen();
                    }
                    auto rc = _sprite->GetRect({0, 0});
                    Regionf uvs(_sprite->GetUVRegion());
                    ImGui::DrawOrigin(_canvas, -1);

                    ImGui::DrawTexture(_canvas,
                                       (ImTextureID)_sprite->GetTexture()->get_texture(),
                                       {rc.x, rc.y},
                                       {rc.x2(), rc.y2()},
                                       {uvs.x1, uvs.y1},
                                       {uvs.x2, uvs.y2},
                                       0xffffffff);

                    ImVec2 org{-_sprite->GetOrigin().x, -_sprite->GetOrigin().y};
                    _canvas.BeginDrag(org, this);

                    if (_canvas.EndDrag(org, this))
                    {
                        _sprite->SetOrigin({-org.x, -org.y});
                    }

                    ImGui::DrawGrid(_canvas);
                    ImGui::DrawRuler(_canvas);
                    ImGui::EndCanvas();
                }

                if (ImGui::LineItem("ftr", { -1, ImGui::GetFrameHeight() })
                    .Spring()
                    .PushStyle(ImStyle_Button, 1)
                    .Text("  Cancel  ")
                    .PopStyle()
                    .Space()
                    .PushStyle(ImStyle_Header, 2)
                    .Text("   Save   ")
                    .PopStyle()
                    .End())
                {
                    if (ImGui::Line().HoverId() == 1)
                        _sprite.reset();
                    if (ImGui::Line().HoverId() == 2 && _sprite->SaveToFile(_sprite->GetPath()))
                        _sprite.reset();
                }
                return !!_sprite;
            };
        };
    } // namespace

    void ResetAtlasCache()
    {
        gAtlasCache.clear();
        gSpriteCache.clear();
    }

    Sprite2D::Ptr LoadSpriteChache(std::string_view spr)
    {
        Sprite2D::Ptr ret;
        auto it = gSpriteCache.find(spr);
        if (it == gSpriteCache.end())
        {
            ret = Sprite2D::LoadShared(spr);
            gSpriteCache[std::string(spr)] = ret;
        }
        else
        {
            return it->second;
        }
        return ret;
    }

    Atlas::Pack LoadSpriteCache(std::string_view atl, std::string_view spr)
    {
        Atlas::Pack out;
        auto        it = gAtlasCache.find(atl);
        if (it == gAtlasCache.end())
        {
            out                   = Atlas::load_shared(atl, spr);
            gAtlasCache[std::string(atl)] = out.atlas;
        }
        else
        {
            out.atlas = it->second;
            if (auto n = out.atlas->find_sprite(spr))
                out.sprite = &out.atlas->get(n);
        }
        return out;
    }

    void ComponentFactory::ImguiExplorer(Scene* scene)
    {
        ResetAtlasCache();
        gFileExplorer.Show(scene, _base_folder);
    }

    void ComponentFactory::SetRoot(const std::string& startPath)
    {
        _base_folder         = startPath;
        gFileExplorer._expanded = false;
        gFileExplorer._path     = startPath;
    }

    std::string ComponentFactory::GetFolderMetadataType(std::string_view folder)
    {
        std::string dir(folder);
        if (dir.empty())
        {
            dir = "./";
        }
        else if (dir.back() != '/' && dir.back() != '\\')
        {
            dir.push_back('/');
        }
        dir.append(MetadataFile);

        if (auto* txt = LoadFileText(dir.c_str()))
        {
            dir.clear();
            std::string_view tpe(txt);
            auto             pos1 = tpe.find('=');
            auto             pos2 = tpe.find('\n');
            if (pos1 != std::string::npos)
            {
                dir = tpe.substr(pos1 + 1, pos2 - pos1);
            }
            UnloadFileText(txt);
            return dir;
        }
        return std::string();
    }

    std::vector<std::string> ComponentFactory::GetFolderFiles(std::string_view folder, std::string_view ext)
    {
        std::vector<std::string> out;
        std::string              dir(folder);
        std::string              ex(ext);

        auto list = LoadDirectoryFilesEx(dir.c_str(), ex.c_str(), false);

        out.reserve(list.count);
        for (int i = 0; i < list.count; i++)
        {
            out.emplace_back(list.paths[i]);
        }
        UnloadDirectoryFiles(list);

        return out;
    }

    bool ComponentFactory::IsAssetFolder(std::string_view path) const
    {
        try
        {
            std::filesystem::path base   = std::filesystem::canonical(_base_folder);
            std::filesystem::path target = std::filesystem::canonical(path);

            auto base_it   = base.begin();
            auto target_it = target.begin();

            for (; base_it != base.end() && target_it != target.end(); ++base_it, ++target_it)
            {
                if (*base_it != *target_it)
                {
                    return false;
                }
            }
            return base_it == base.end();
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            return false;
        }
        return false;
    }

    bool ComponentFactory::NormalizeAssetPath(std::string& str) const
    {
        try
        {
            std::filesystem::path base      = std::filesystem::canonical(_base_folder);
            std::filesystem::path target    = std::filesystem::canonical(str);
            auto                  base_it   = base.begin();
            auto                  target_it = target.begin();

            for (; base_it != base.end() && target_it != target.end(); ++base_it, ++target_it)
            {
                if (*base_it != *target_it)
                {
                    return false;
                }
            }
            if (base_it == base.end())
            {
                str = _base_folder;
                str.pop_back();
                for (; target_it != target.end(); ++target_it)
                {
                    str.push_back('/');
                    str.append(target_it->string());
                }
                return true;
            }
        }

        catch (const std::filesystem::filesystem_error& e)
        {
            return false;
        }
        return false;
    }

    void FileExplorer::Show(Scene* scene, StringView base_folder)
    {
        if (!_expanded)
        {
            _expanded = true;
            _dirs.clear();
            _files.clear();
            _sprites.clear();

            if (_path != base_folder)
            {
                _dirs.push_back("..");
            }

            _metatype = ComponentFactory::GetFolderMetadataType(_path);
            for (const auto& entry : std::filesystem::directory_iterator(_path))
            {
                if (entry.is_directory())
                {
                    _dirs.push_back(entry.path().filename().string());
                }
                else if (entry.is_regular_file())
                {
                    _files.push_back(entry.path().filename().string());
                }
            }
        }

        if (_editor)
        {
            if (ImGui::LineSelect(ImGui::GetID(".."), false)
                    .Space()
                    .PushColor(0xff52d1ff)
                    .Text(ICON_FA_FOLDER)
                    .PopColor()
                    .Space()
                    .Text("..")
                    .End() ||
                !_editor->imgui_show())
            {
                _editor.reset();
            }
        }
        else
        {
            std::string_view context1;
            for (auto& dir : _dirs)
            {
                if (ImGui::LineSelect(ImGui::GetID(dir.c_str()), false)
                        .Space()
                        .PushColor(0xff52d1ff)
                        .Text(ICON_FA_FOLDER)
                        .PopColor()
                        .Space()
                        .Text(dir.c_str())
                        .End())
                {
                    _expanded = false;
                    if (dir == "..")
                    {
                        _path.pop_back();
                        _path.resize(_path.rfind('/') + 1);
                    }
                    else
                    {
                        _path += dir;
                        _path += '/';
                    }
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
                {
                    context1 = dir;
                }
            }

            if (_metatype.empty())
            {
                ShowFilesRaw(scene);
            }
            else
            {
                ShowFilesMeta(scene, _metatype);
            }

            if (!context1.empty())
            {
                ImGui::OpenPopup("FolderContextMenu");
                _context = context1;
            }
            if (ImGui::BeginPopup("FolderContextMenu"))
            {
                if (ImGui::MenuItem("Pack folder Sprites"))
                {
                    std::string dir(_path);
                    if (_context != "..")
                        dir.append(_context).push_back('/');
                    Sprite2D::CreateTextureAtlas(dir, "", 4096, 4096);
                }
                ImGui::EndPopup();
            }
        }
    }

    void FileExplorer::ShowFilesRaw(Scene* scene)
    {
        std::string_view context2;
        for (auto& file : _files)
        {
            uint32_t clr;
            auto     ico = GetFileIcon(file, clr);

            if (ImGui::LineSelect(ImGui::GetID(file.c_str()), false)
                    .Space()
                    .PushColor(clr)
                    .Text(ico)
                    .PopColor()
                    .Space()
                    .Text(file.c_str())
                    .End())
            {
                _editor = ImGui::Editor::load_from_file(_path + file);
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                std::string_view ext(file);
                ext = ext.substr(ext.rfind('.') + 1);
                if (ext == "map")
                {
                    scene->Load(_path + file);
                }
                else if (ext == "shader")
                {
                    auto shader = Shader2D::LoadShared(_path + file);
                    ImGui::Dialog::ShowOnce<ShaderProperties>("Shader Properties", {800, 600}, 0, shader);
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1))
            {
                context2 = file;
            }
        }

        if (!context2.empty())
        {
            ImGui::OpenPopup("FileContextMenu");
            _context = context2;
        }
        if (ImGui::BeginPopup("FileContextMenu"))
        {
            ImGui::EndPopup();
        }
    }

    void FileExplorer::ShowFilesMeta(Scene* scene, StringView type)
    {
        if (type == std::string_view("atlas"))
        {
            return ShowFilesAtlas(scene);
        }

        ShowFilesRaw(scene);
    }

    void FileExplorer::ShowFilesAtlas(Scene* scene)
    {
        if (_sprites.empty())
        {
            size_t index = 0;
            for (auto& el : _files)
            {
                if (GetFileExt(el) == ".sprite")
                {
                    _sprites.emplace_back(Sprite2D::LoadShared(_path + el), index);
                }
                ++index;
            }
        }

        ShowGrid(_sprites);
    }

    void FileExplorer::ShowGrid(auto& items)
    {
        float text_height     = ImGui::GetTextLineHeightWithSpacing();
        float item_height     = _icon_size + text_height + 4.0f;
        float avail_x         = ImGui::GetContentRegionAvail().x;
        float item_full_width = _icon_size + _padding;
        float x               = 0.0f;

        // Check if the child window is hovered
        bool     is_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        ImGuiIO& io         = ImGui::GetIO();
        if (is_hovered && io.KeyShift && io.MouseWheel != 0.f)
        {
            _icon_size = std::clamp(_icon_size + io.MouseWheel * 4.0f, 32.0f, 256.0f);
        }

        static IconData data{};
        size_t n = 0;
        for (auto& el : items)
        {
            ImGui::PushID(n);
            // Start new line if not enough space
            if (x + item_full_width > avail_x)
            {
                x = 0.0f;
                ImGui::NewLine();
            }
            else if (x > 0.0f)
            {
                ImGui::SameLine();
            }

            const auto selected = _selected == n;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

            GetItem(el, data);

            ImGui::BeginGroup();
            if (ImGui::ImageButton("txt",
                                   data.txt,
                                   data.size,
                                   ImVec2(data.uv.x1, data.uv.y1),
                                   ImVec2(data.uv.x2, data.uv.y2)))
            {
                _selected = n;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                OpenItem(el);
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                _selected = n;
                ImGui::SetDragDropPayload(data.type, &data.drag, data.drag_size);
                ImGui::EndDragDropSource();
            }

            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + _icon_size);
            ImGui::TextWrapped(data.label);
            ImGui::PopTextWrapPos();

            if (selected)
                ImGui::PopStyleColor();
            ImGui::EndGroup();

            ImGui::PopID();
            x += item_full_width;
            ++n;
        }
    }

    void FileExplorer::GetItem(const std::pair<Sprite2D::Ptr, size_t>& el, IconData& data)
    {
        const auto size = el.first->GetSize();
        const auto ms   = _icon_size / std::max(size.x, size.y);
        data.txt        = (ImTextureID)el.first->GetTexture()->get_texture();
        data.uv         = el.first->GetUVRegion();
        data.size       = ImVec2(size.x * ms, size.y * ms);
        data.label      = _files[el.second].c_str();
        data.drag       = el.first.get();
        data.drag_size  = sizeof(Sprite2D*);
        data.type       = "SPRITE2D";
    }

    void FileExplorer::OpenItem(const std::pair<Sprite2D::Ptr, size_t>& item)
    {
        ImGui::Dialog::ShowOnce<SpriteProperties>("Sprite Properties", {800, 600}, 0, item.first);
    }



} // namespace fin
