#pragma once

#include "include.hpp"

namespace fin
{
    class Texture2D;
    class Shader2D;

    class Renderer
    {
    public:
        Renderer()                = default;
        ~Renderer()               = default;
        Renderer(const Renderer&) = delete;

        void set_debug(bool debug);
        bool is_debug() const;
        void set_origin(Vec2f origin);
        void set_color(Color clr);
        void render_texture(const Texture* texture, const Rectf& source, const Rectf& dest);
        void render_line(Vec2f from, Vec2f to);
        void render_line(float fromx, float fromy, float tox, float toy);
        void render_line_rect(const Rectf& dest);
        void render_rect(const Rectf& dest);
        void render_polyline(const Vec2f* pont, size_t size, bool closed, Vec2f offset);
        void render_triangle(Vec2f a, Vec2f b, Vec2f c);
        void render_line_circle(Vec2f pos, float radius);
        void render_circle(Vec2f pos, float radius);
        void render_debug_text(Vec2f to, const char* fmt, ...);

        void SetShader(Shader2D* txt);
        void RenderTexture(const Texture2D* txt, const Regionf& pos, const Regionf& uv);

        bool     _debug{};
        Color    _color{WHITE};
        Camera2D _camera{{}, {}, 0, 1.f};
    };
}
