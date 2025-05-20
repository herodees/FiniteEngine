#include "shared_resource.hpp"

#include "atlas.hpp"

namespace fin
{
    struct SharedResource
    {
        std::string                                                                                    _path;
        std::unordered_map<std::string, std::weak_ptr<Atlas>, std::string_hash, std::equal_to<>>       _atlases;
        std::unordered_map<std::string, std::weak_ptr<Texture2D>, std::string_hash, std::equal_to<>>   _textures;
        std::unordered_map<std::string, std::weak_ptr<Surface>, std::string_hash, std::equal_to<>>     _surfaces;
        std::unordered_map<std::string, std::weak_ptr<SoundSource>, std::string_hash, std::equal_to<>> _sounds;
    };

    static SharedResource _shared_res;

    Surface::Surface(Surface&& s) noexcept : surface{}
    {
        std::swap(surface, s.surface);
    }

    Surface::~Surface()
    {
        clear();
    }

    Surface& Surface::operator=(Surface&& s) noexcept
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

    bool Surface::load_from_file(const std::filesystem::path& filePath)
    {
        clear();
        surface = LoadImage(filePath.string().c_str());
        return surface.data;
    }

    bool Surface::load_from_pixels(int width, int height, int format, const void* pixels)
    {
        clear();
        const auto size = GetPixelDataSize(width, height, format);
        surface.width   = width;
        surface.height  = height;
        surface.format  = format;
        surface.mipmaps = 1;
        surface.data    = RL_MALLOC(size);
        std::memcpy(surface.data, pixels, size);

        return surface.data;
    }

    Surface Surface::create_sub_surface(int x, int y, int w, int h) const
    {
        if (!surface.data) // Ensure the original surface is valid
            return Surface();

        const auto size = GetPixelDataSize(w, h, surface.format);
        Surface    out;
        out.surface.width   = w;
        out.surface.height  = h;
        out.surface.format  = surface.format;
        out.surface.mipmaps = 1;
        out.surface.data    = RL_MALLOC(size);

        ImageDraw(&out.surface, surface, {(float)x, (float)y, (float)w, (float)h}, {0, 0, (float)w, (float)h}, WHITE);

        return out;
    }

    const unsigned char* Surface::data() const
    {
        return (const unsigned char*)surface.data;
    }

    int Surface::get_data_size() const
    {
        return GetPixelDataSize(surface.width, surface.height, surface.format);
    }

    Surface::Ptr Surface::load_shared(std::string_view pth)
    {
        auto it = _shared_res._surfaces.find(pth);
        if (it != _shared_res._surfaces.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                                = std::make_shared<Surface>();
        _shared_res._surfaces[std::string(pth)] = ptr;
        ptr->load_from_file(pth);

        return ptr;
    }

    Surface::operator bool() const
    {
        return surface.data;
    }


    Texture2D::Texture2D(const std::filesystem::path& filePath)
    {
        load_from_file(filePath);
    }

    Texture2D::~Texture2D()
    {
        clear();
    }

    Texture2D::Texture2D(Texture2D&& other) noexcept
    {
        std::swap(texture, other.texture);
    }

    Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
    {
        std::swap(texture, other.texture);
        return *this;
    }

    void Texture2D::clear()
    {
        path.clear();
        bitmask.clear();
        if (texture.id)
        {
            UnloadTexture(texture);
            texture = {};
        }
    }

    bool Texture2D::load_from_file(const std::filesystem::path& filePath)
    {
        clear(); // Free existing texture if any
        path     = filePath.string();
        auto img = LoadImage(path.c_str());
        generate_alpha_mask(img, 0.5f);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);

        if (!texture.id)
        {
            return false;
        }

        return true;
    }

    bool Texture2D::load_from_surface(const Surface& loadedSurface)
    {
        clear(); // Free existing texture if any

        texture = LoadTextureFromImage(*loadedSurface.get_surface());
        generate_alpha_mask(*loadedSurface.get_surface(), 0.5f);

        if (!texture.id)
        {
            return false;
        }

        return true;
    }

    bool Texture2D::update_texture_data(const void* pixels)
    {
        if (!texture.id)
            return false;

        UpdateTexture(texture, pixels);

        return true;
    }

    bool Texture2D::update_texture_data(const void* pixels, const Rectf& rc)
    {
        if (!texture.id)
            return false;

        UpdateTextureRec(texture, (::Rectangle&)(rc), pixels);

        return true;
    }

    void Texture2D::set_repeat(bool r)
    {
        SetTextureWrap(texture, r ? TEXTURE_WRAP_REPEAT : TEXTURE_WRAP_CLAMP);
    }

    bool Texture2D::is_alpha_visible(uint32_t x, uint32_t y) const
    {
        if (x >= texture.width || y >= texture.height)
            return false;
        int idx = y * texture.width + x;
        return (bitmask[idx / 8] >> (idx % 8)) & 1;
    }

    void Texture2D::generate_alpha_mask(const Image& img, float threshold)
    {
        const int w     = img.width;
        const int h     = img.height;
        const int total = w * h;

        bitmask.clear();
        bitmask.resize((total + 7) / 8, 0);

        if (img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
        {
            const uint8_t* data           = static_cast<const uint8_t*>(img.data);
            const uint8_t  alphaThreshold = static_cast<uint8_t>(threshold * 255.0f);

            for (int i = 0; i < total; ++i)
            {
                uint8_t a = data[i * 4 + 3]; // A channel
                if (a >= alphaThreshold)
                    bitmask[i / 8] |= (1 << (i % 8));
            }
        }
        else
        {
            // Assume solid image if not RGBA8 — mark all as opaque
            std::fill(bitmask.begin(), bitmask.end(), 0xFF);
        }
    }

    Texture2D::Ptr Texture2D::load_shared(std::string_view pth)
    {
        auto it = _shared_res._textures.find(pth);
        if (it != _shared_res._textures.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                                = std::make_shared<Texture2D>();
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


    Atlas::Ptr Atlas::load_shared(std::string_view pth)
    {
        auto it = _shared_res._atlases.find(pth);
        if (it != _shared_res._atlases.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                               = std::make_shared<Atlas>();
        _shared_res._atlases[std::string(pth)] = ptr;
        ptr->load_from_file(pth);

        return ptr;
    }

    Atlas::Pack Atlas::load_shared(std::string_view pth, std::string_view spr)
    {
        Pack out;
        if (out.atlas = load_shared(pth))
        {
            if (auto n = out.atlas->find_sprite(spr))
            {
                out.sprite = &out.atlas->get(n);
            }
        }
        return out;
    }

    SoundSource::SoundSource(SoundSource&& other) noexcept
    {
        std::swap(sound, other.sound);
    }

    SoundSource::~SoundSource()
    {
        clear();
    }

    SoundSource& SoundSource::operator=(SoundSource&& other) noexcept
    {
        std::swap(sound, other.sound);
        return *this;
    }

    void SoundSource::clear()
    {
        if (sound.stream.buffer)
        {
            UnloadSound(sound);
            sound = {};
        }
    }

    const Sound* SoundSource::get_sound() const
    {
        return &sound;
    }

    bool SoundSource::load_from_file(const std::filesystem::path& filePath)
    {
        _path = filePath.string();
        clear(); // Free existing texture if any
        sound = LoadSound(_path.c_str());

        if (!sound.stream.buffer)
        {
            return false;
        }

        return true;
    }

    const std::string& SoundSource::get_path() const
    {
        return _path;
    }

    SoundSource::Ptr SoundSource::load_shared(std::string_view pth)
    {
        auto it = _shared_res._sounds.find(pth);
        if (it != _shared_res._sounds.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                              = std::make_shared<SoundSource>();
        _shared_res._sounds[std::string(pth)] = ptr;
        ptr->load_from_file(pth);

        return ptr;
    }

    SoundSource::operator bool() const
    {
        return sound.stream.buffer;
    }

} // namespace fin
