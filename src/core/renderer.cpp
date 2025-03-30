#include "renderer.hpp"
#include <rlgl.h>

namespace fin
{

void Renderer::set_origin(Vec2f origin)
{
    _camera.offset.x = -origin.x;
    _camera.offset.y = -origin.y;
}

void Renderer::set_color(Color clr)
{
    _color = clr;
}

void Renderer::render_texture(const Texture *texture, const Rectf &source, const Rectf &dest)
{
    const auto width = texture->width;
    const auto height = texture->height;

    rlSetTexture(texture->id);
    rlBegin(RL_QUADS);

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

void Renderer::render_debug_text(Vec2f to, const char *fmt, ...)
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

}
