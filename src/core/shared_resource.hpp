#pragma once

#include "include.hpp"

namespace fin
{

    class Surface : std::enable_shared_from_this<Surface>
    {
    private:
        Image surface{};

    public:
        using Ptr = std::shared_ptr<Surface>;

        Surface() = default;
        Surface(Surface&& s) noexcept;
        Surface(const Surface&) = delete;
        ~Surface();
        Surface& operator=(Surface&& s) noexcept;
        Surface& operator=(const Surface&) = delete;
        void     clear();
        bool     load_from_file(const std::filesystem::path& filePath);
        bool     load_from_pixels(int width, int height, int format, const void* pixels);
        Surface  create_sub_surface(int x, int y, int w, int h) const;

        Vec2i get_size() const
        {
            return Vec2i{surface.width, surface.height};
        }
        int get_width() const
        {
            return surface.width;
        }
        int get_height() const
        {
            return surface.height;
        }
        const unsigned char* data() const;
        int                  get_data_size() const;

        const Image* get_surface() const
        {
            return &surface;
        }

        static Ptr load_shared(std::string_view pth);
        explicit   operator bool() const;
    };


    class Texture2D : std::enable_shared_from_this<Texture2D>
    {
    private:
        Texture              texture{};
        std::string          path;
        std::vector<uint8_t> bitmask;

        void generate_alpha_mask(const Image& img, float threshold);

    public:
        using Ptr = std::shared_ptr<Texture2D>;

        Texture2D() = default;
        Texture2D(const std::filesystem::path& filePath);
        ~Texture2D();
        Texture2D(const Texture2D&)            = delete;
        Texture2D& operator=(const Texture2D&) = delete;
        Texture2D(Texture2D&& other) noexcept;
        Texture2D& operator=(Texture2D&& other) noexcept;
        void       clear();

        bool load_from_file(const std::filesystem::path& filePath);
        bool load_from_surface(const Surface& loadedSurface);
        bool update_texture_data(const void* pixels);
        bool update_texture_data(const void* pixels, const Rectf& rc);
        void set_repeat(bool r);

        const std::string& get_path() const
        {
            return path;
        }

        Vec2i get_size() const
        {
            return Vec2i{texture.width, texture.height};
        }
        int get_width() const
        {
            return texture.width;
        }
        int get_height() const
        {
            return texture.height;
        }
        const Texture* get_texture() const
        {
            return &texture;
        }

        bool is_alpha_visible(uint32_t x, uint32_t y) const;

        static Ptr load_shared(std::string_view pth);
        explicit   operator bool() const;
    };


    class RenderTexture2D
    {
    public:
        RenderTexture texture{};
        RenderTexture2D() = default;
        ~RenderTexture2D();
        RenderTexture2D(const RenderTexture2D&)            = delete;
        RenderTexture2D& operator=(const RenderTexture2D&) = delete;
        void             clear();
        void             create(int w, int h);

        Vec2i get_size() const
        {
            return Vec2i{texture.texture.width, texture.texture.height};
        }
        int get_width() const
        {
            return texture.texture.width;
        }
        int get_height() const
        {
            return texture.texture.height;
        }
        const RenderTexture* get_texture() const
        {
            return &texture;
        }

        explicit operator bool() const;
    };


    class SoundSource : std::enable_shared_from_this<SoundSource>
    {
    private:
        Sound sound{};
        std::string _path;

    public:
        using Ptr = std::shared_ptr<SoundSource>;

        SoundSource() = default;
        SoundSource(SoundSource&& s) noexcept;
        SoundSource(const SoundSource&) = delete;
        ~SoundSource();
        SoundSource& operator=(SoundSource&& s) noexcept;
        SoundSource& operator=(const SoundSource&) = delete;
        void         clear();
        const Sound* get_sound() const;
        bool         load_from_file(const std::filesystem::path& filePath);
        const std::string& get_path() const;

        static Ptr load_shared(std::string_view pth);

        explicit operator bool() const;
    };

} // namespace fin
