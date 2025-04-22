#pragma once

#include "include.hpp"

namespace fin
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;
        Renderer(const Renderer&) = delete;

        void set_origin(Vec2f origin);
        void set_color(Color clr);
        void render_texture(const Texture* texture, const Rectf &source, const Rectf &dest);
        void render_line(Vec2f from, Vec2f to);
        void render_line(float fromx, float fromy, float tox, float toy);
        void render_line_rect(const Rectf& dest);
        void render_rect(const Rectf& dest);
        void render_line_circle(Vec2f pos, float radius);
        void render_circle(Vec2f pos, float radius);
        void render_debug_text(Vec2f to, const char *fmt, ...);

        Color _color{WHITE};
        Camera2D _camera{{}, {}, 0, 1.f};
    };
}
