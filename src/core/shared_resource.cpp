#include "shared_resource.hpp"

namespace fin
{
struct SharedResource
{
    std::string _path;
 //   std::unordered_map<std::string, std::weak_ptr<Atlas>, std::string_hash, std::equal_to<>>
      //  _atlases;
    std::unordered_map<std::string, std::weak_ptr<Texture2D>, std::string_hash, std::equal_to<>>
        _textures;
    std::unordered_map<std::string, std::weak_ptr<Surface>, std::string_hash, std::equal_to<>>
        _surfaces;
};

static SharedResource _shared_res;

Surface::Surface(Surface &&s) noexcept : surface{}
{
    std::swap(surface, s.surface);
}

Surface::~Surface()
{
    clear();
}

Surface &Surface::operator=(Surface &&s) noexcept
{
    std::swap(surface, s.surface);
    return *this;
}

void Surface::clear()
{
    if (surface.data)
    {
        UnloadImage(surface);
    }
    surface = {};
}

bool Surface::load_from_file(const std::filesystem::path &filePath)
{
    clear();
    surface = LoadImage(filePath.string().c_str());
    return surface.data;
}

bool Surface::load_from_pixels(int width, int height, int format, const void *pixels)
{
    clear();
    const auto size = GetPixelDataSize(width, height, format);
    surface.width = width;
    surface.height = height;
    surface.format = format;
    surface.mipmaps = 1;
    surface.data = RL_MALLOC(size);
    std::memcpy(surface.data, pixels, size);

    return surface.data;
}

Surface Surface::create_sub_surface(int x, int y, int w, int h) const
{
    if (!surface.data) // Ensure the original surface is valid
        return Surface();

    Surface out;
    out.surface = ImageFromImage(surface, {float(x), float(y), float(w), float(h)});
    return out;
}

std::shared_ptr<Surface> Surface::load_shared(std::string_view pth)
{
    auto it = _shared_res._surfaces.find(pth);
    if (it != _shared_res._surfaces.end() && !it->second.expired())
        return it->second.lock();

    auto ptr = std::make_shared<Surface>();
    _shared_res._surfaces[std::string(pth)] = ptr;
    ptr->load_from_file(pth);

    return ptr;
}

Surface::operator bool() const
{
    return surface.data;
}




Texture2D::Texture2D(const std::filesystem::path &filePath)
{
    load_from_file(filePath);
}

Texture2D::~Texture2D()
{
    clear();
}

Texture2D::Texture2D(Texture2D &&other) noexcept
{
    std::swap(texture, other.texture);
}

Texture2D &Texture2D::operator=(Texture2D &&other) noexcept
{
    std::swap(texture, other.texture);
    return *this;
}

void Texture2D::clear()
{
    if (texture.id)
    {
        UnloadTexture(texture);
        texture = {};
    }
}

bool Texture2D::load_from_file(const std::filesystem::path &filePath)
{
    clear(); // Free existing texture if any
    texture = LoadTexture(filePath.string().c_str());

    if (!texture.id)
    {
        return false;
    }

    return true;
}

bool Texture2D::load_from_surface(const Surface &loadedSurface)
{
    clear(); // Free existing texture if any

    texture = LoadTextureFromImage(*loadedSurface.get_surface());

    if (!texture.id)
    {
        return false;
    }

    return true;
}

bool Texture2D::update_texture_data(const void *pixels)
{
    if (!texture.id)
        return false;

    UpdateTexture(texture, pixels);

    return true;
}

bool Texture2D::update_texture_data(const void *pixels, const Rectf &rc)
{
    if (!texture.id)
        return false;

    UpdateTextureRec(texture, (::Rectangle&)(rc), pixels);

    return true;
}

std::shared_ptr<Texture2D> Texture2D::load_shared(std::string_view pth)
{
    auto it = _shared_res._textures.find(pth);
    if (it != _shared_res._textures.end() && !it->second.expired())
        return it->second.lock();

    auto ptr = std::make_shared<Texture2D>();
    _shared_res._textures[std::string(pth)] = ptr;
    ptr->load_from_file(pth);

    return ptr;
}

Texture2D::operator bool() const
{
    return texture.id;
}

RenderTexture2D::~RenderTexture2D()
{
    clear();
}

void RenderTexture2D::clear()
{
    if (texture.id)
    {
        UnloadRenderTexture(texture);
        texture = {};
    }
}

void RenderTexture2D::create(int w, int h)
{
    clear();
    texture = LoadRenderTexture(w, h);
}

RenderTexture2D::operator bool() const
{
    return texture.id;
}

} // namespace fin
