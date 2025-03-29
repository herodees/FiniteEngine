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
        void render_texture(const Texture* texture, const Rectf &source, const Rectf &dest);
        void render_line(Vec2f from, Vec2f to, Color clr);
        void render_line(float fromx, float fromy, float tox, float toy, Color clr);

        Color _color{WHITE};
        Camera2D _camera;
    };
}
