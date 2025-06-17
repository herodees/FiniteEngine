#pragma once

#include "core/atlas.hpp"
#include "include.hpp"
#include "misc/cpp/imgui_stdlib.h"

namespace ImGui
{
    bool SpriteInput(const char* label, fin::Atlas::Pack* pack);
    bool TextureInput(const char* label, fin::Texture2D::Ptr* pack);
    bool SoundInput(const char* label, fin::SoundSource::Ptr* pack);
    bool PointVector(const char* label, std::vector<fin::Vec2f>* points, ImVec2 size, int* active = nullptr);
    bool PointVector(const char* label, fin::msg::Var* points, ImVec2 size, int* active = nullptr);
    void SpriteImage(fin::Atlas::Sprite* spr, ImVec2 size);

    struct CanvasParams
    {
        ImVec2 origin   = ImVec2(0, 0); // pan offset
        float  zoom     = 1.0f;         // zoom factor
        float  min_zoom = 0.05f;
        float  max_zoom = 8.0f;
        ImVec2 mouse_pos_canvas; // mouse position in canvas local
        ImVec2 mouse_pos_world;  // transformed to world space
        ImVec2 canvas_pos;       // top-left of canvas in screen space
        ImVec2 canvas_size;      // canvas size
        ImVec2 snap_grid          = ImVec2(0, 0);
        ImVec2 drag_offset_world  = ImVec2(0, 0);
        bool   dragging           = false;
        void*  dragging_user_data = nullptr;

        ImVec2 WorldToScreen(ImVec2 world) const;
        ImVec2 ScreenToWorld(ImVec2 screen) const;
        ImVec2 Snap(ImVec2 world) const;
        ImVec2 SnapScreenToWorld(ImVec2 screen) const;
        void   CenterOrigin();
        bool   DragPoint(ImVec2& pt, void* user_data, float radius_screen);
        bool   BeginDrag(ImVec2& pt, void* user_data);
        bool   EndDrag(ImVec2& pt, void* user_data);
    };
    bool BeginCanvas(const char* id, ImVec2 size, CanvasParams& params);
    void EndCanvas();
    void DrawRuler(const CanvasParams& params, ImVec2 grid_step = ImVec2(32, 32), ImU32 color = IM_COL32(255, 255, 255, 180));
    void DrawGrid(const CanvasParams& params, ImVec2 grid_step = ImVec2(32, 32), ImU32 color = IM_COL32(255, 255, 255, 30));
    void DrawOrigin(const CanvasParams& params, float axis_length = 100.0f);
    void DrawTexture(const CanvasParams& params, ImTextureID tid, ImVec2 p1, ImVec2 p2, ImVec2 uv1, ImVec2 uv2, ImU32 color);

    bool OpenFileInput(const char* label, std::string& path, const char* filter);
    bool SaveFileInput(const char* label, std::string& path, const char* filter);

    void HelpMarker(const char* desc);

    const char* FormatStr(const char* fmt, ...);

    void  SetDragData(void* d, const char* id = nullptr);
    void* GetDragData(const char* id = nullptr);

    bool FileMenu(std::string& path, const char* filter);
    bool OpenFileName(const char* label, std::string& path, const char* filter);

    class Editor : std::enable_shared_from_this<Editor>
    {
    public:
        static std::shared_ptr<Editor> load_from_file(std::string_view path);

        Editor()          = default;
        virtual ~Editor() = default;

        virtual bool Load(std::string_view path) = 0;
        virtual bool imgui_show()                = 0;
    };

    class Dialog : std::enable_shared_from_this<Editor>
    {
    public:
        using Ptr = std::shared_ptr<Dialog>;

        virtual ~Dialog() = default;

        template <typename T, typename... Args>
        static Ptr Create(Args&&... args)
        {
            static_assert(std::is_base_of_v<Dialog, T>,
                          "Dialog::Create can only be used with types derived from Dialog");

            return std::make_shared<T>(std::forward<Args>(args)...); // Fixed argument forwarding
        }

        template <typename T, typename... Args>
        static void Show(const char* label, ImVec2 size, ImGuiWindowFlags flags, Args&&... args)
        {
            if (auto dialog = Create<T>(std::forward<Args>(args)...))
            {
                Show(label, size, flags, dialog); // Call the existing Show(Ptr) method
            }
        }

        template <typename T, typename... Args>
        static void ShowOnce(const char* label, ImVec2 size, ImGuiWindowFlags flags, Args&&... args)
        {
            for (auto el : s_activeDialogs)
            {
                if (el.label == label)
                    return;
            }
            if (auto dialog = Create<T>(std::forward<Args>(args)...))
            {
                Show(label, size, flags, dialog); // Call the existing Show(Ptr) method
            }
        }

        virtual bool OnUpdate()
        {
            return true;
        };

        static void Show(const char* label, ImVec2 size, ImGuiWindowFlags flags, Ptr dialog);
        static void Update();
        static void Close(const Ptr& dialog);

    private:
        struct Info
        {
            std::string      label;
            ImVec2           size;
            ImGuiWindowFlags flags;
            Ptr              dialog;
        };
        static std::vector<Info> s_activeDialogs; // Track active dialogs
    };

} // namespace ImGui
