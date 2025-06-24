#include "renderer.hpp"
#include "shared_resource.hpp"

#include <rlgl.h>

namespace fin
{
    void Renderer::set_debug(bool debug)
    {
        _debug = debug;
    }

    void Renderer::set_origin(Vec2f origin)
    {
        _camera.offset.x = -origin.x;
        _camera.offset.y = -origin.y;
    }

    void Renderer::set_color(Color clr)
    {
        _color = clr;
    }

    void Renderer::render_texture(const Texture* texture, const Rectf& source, const Rectf& dest)
    {
        const auto width  = texture->width;
        const auto height = texture->height;


        rlBegin(RL_QUADS);

        rlSetTexture(texture->id);
        rlColor4ub(_color.r, _color.g, _color.b, _color.a);
        rlNormal3f(0.0f, 0.0f, 1.0f); // Normal vector pointing towards viewer

        // Top-left corner for texture and quad
        rlTexCoord2f(source.x / width, source.y / height);
        rlVertex2f(dest.x, dest.y);

        // Bottom-left corner for texture and quad
        rlTexCoord2f(source.x / width, (source.y + source.height) / height);
        rlVertex2f(dest.x, dest.y2());

        // Bottom-right corner for texture and quad
        rlTexCoord2f((source.x + source.width) / width, (source.y + source.height) / height);
        rlVertex2f(dest.x2(), dest.y2());

        // Top-right corner for texture and quad
        rlTexCoord2f((source.x + source.width) / width, source.y / height);
        rlVertex2f(dest.x2(), dest.y);

        rlEnd();
        rlSetTexture(0);
    }

    void Renderer::render_line(Vec2f from, Vec2f to)
    {
        DrawLine(from.x, from.y, to.x, to.y, _color);
    }

    void Renderer::render_line(float fromx, float fromy, float tox, float toy)
    {
        DrawLine(fromx, fromy, tox, toy, _color);
    }

    void Renderer::render_line_rect(const Rectf& dest)
    {
        DrawRectangleLinesEx({dest.x, dest.y, dest.width, dest.height}, 1, _color);
    }

    void Renderer::render_rect(const Rectf& dest)
    {
        DrawRectangleV({dest.x, dest.y}, {dest.width, dest.height}, _color);
    }

    void Renderer::render_polyline(const Vec2f* point, size_t size, bool closed, Vec2f offset)
    {
        if (size < 1)
            return;

        for (size_t n = 0; n < size - 1; ++n)
        {
            auto from = point[n] + offset;
            auto to   = point[n + 1] + offset;
            DrawLine(from.x, from.y, to.x, to.y, _color);
        }

        if (closed)
        {
            auto from = point[0] + offset;
            auto to   = point[size - 1] + offset;
            DrawLine(from.x, from.y, to.x, to.y, _color);
        }

    }

    void Renderer::render_triangle(Vec2f a, Vec2f b, Vec2f c)
    {
        Color color = {_color.r, _color.g, _color.b, _color.a};
        DrawTriangle({a.x, a.y}, {b.x, b.y}, {c.x, c.y}, color);
    }

    void Renderer::render_line_circle(Vec2f pos, float radius)
    {
        DrawCircleLinesV({pos.x, pos.y}, radius, _color);
    }

    void Renderer::render_circle(Vec2f pos, float radius)
    {
        DrawCircleV({pos.x, pos.y}, radius, _color);
    }

    void Renderer::render_debug_text(Vec2f to, const char* fmt, ...)
    {
        static char buf[512];

        va_list args;
        va_start(args, fmt);
        int w = vsnprintf(buf, std::size(buf), fmt, args);
        va_end(args);

        if (w == -1 || w >= (int)std::size(buf))
            w = (int)std::size(buf) - 1;
        buf[w] = 0;

        DrawText(buf, to.x, to.y, 10, _color);
    }

    void Renderer::SetShader(Shader2D* txt)
    {
        Shader2D::Bind(txt);
    }

    void Renderer::RenderTexture(const Texture2D* txt, const Regionf& pos, const Regionf& uv)
    {
        if (!txt)
            return;

        rlBegin(RL_QUADS);

        rlSetTexture(txt->get_texture()->id);
        rlColor4ub(_color.r, _color.g, _color.b, _color.a);
        rlNormal3f(0.0f, 0.0f, 1.0f); // Normal vector pointing towards viewer

        // Top-left corner for texture and quad
        rlTexCoord2f(uv.x1, uv.y1);
        rlVertex2f(pos.x1, pos.y1);

        // Bottom-left corner for texture and quad
        rlTexCoord2f(uv.x1, uv.y2);
        rlVertex2f(pos.x1, pos.y2);

        // Bottom-right corner for texture and quad
        rlTexCoord2f(uv.x2, uv.y2);
        rlVertex2f(pos.x2, pos.y2);

        // Top-right corner for texture and quad
        rlTexCoord2f(uv.x2, uv.y1);
        rlVertex2f(pos.x2, pos.y1);

        rlEnd();
        rlSetTexture(0);
    }

    bool Renderer::is_debug() const
    {
        return _debug;
    }

} // namespace fin
