#pragma once

#include "include.hpp"

namespace fin
{

class Surface : std::enable_shared_from_this<Surface>
{
private:
    Image surface{};

public:
    Surface() = default;
    Surface(Surface &&s) noexcept;
    Surface(const Surface &) = delete;
    ~Surface();
    Surface &operator=(Surface &&s) noexcept;
    Surface &operator=(const Surface &) = delete;
    void clear();
    bool load_from_file(const std::filesystem::path &filePath);
    bool load_from_pixels(int width, int height, int format, const void *pixels);
    Surface create_sub_surface(int x, int y, int w, int h) const;

    Vec2i get_size() const { return Vec2i{surface.width, surface.height}; }
    int get_width() const{ return surface.width; }
    int get_height() const{ return surface.height; }
    const Image *get_surface() const { return &surface; }

    static std::shared_ptr<Surface> load_shared(std::string_view pth);
    explicit operator bool() const;
};



class Texture2D : std::enable_shared_from_this<Texture2D>
{
private:
    Texture texture{};

public:
    Texture2D() = default;
    Texture2D(const std::filesystem::path &filePath);
    ~Texture2D();
    Texture2D(const Texture2D &) = delete;
    Texture2D &operator=(const Texture2D &) = delete;
    Texture2D(Texture2D &&other) noexcept;
    Texture2D &operator=(Texture2D &&other) noexcept;
    void clear();

    bool load_from_file(const std::filesystem::path &filePath);
    bool load_from_surface(const Surface &loadedSurface);
    bool update_texture_data(const void *pixels);
    bool update_texture_data(const void *pixels, const Rectf &rc);

    Vec2i get_size() const{ return Vec2i{texture.width, texture.height}; }
    int get_width() const{ return texture.width; }
    int get_height() const{ return texture.height; }
    const Texture *get_texture() const { return &texture; }

    static std::shared_ptr<Texture2D> load_shared(std::string_view pth);
    explicit operator bool() const;
};



class RenderTexture2D
{
public:
    RenderTexture texture{};
    RenderTexture2D() = default;
    ~RenderTexture2D();
    RenderTexture2D(const RenderTexture2D &) = delete;
    RenderTexture2D &operator=(const RenderTexture2D &) = delete;
    void clear();
    void create(int w, int h);

    Vec2i get_size() const{ return Vec2i{texture.texture.width, texture.texture.height}; }
    int get_width() const{ return texture.texture.width; }
    int get_height() const{ return texture.texture.height; }
    const RenderTexture *get_texture() const{ return &texture; }

    explicit operator bool() const;
};

} // namespace fin
