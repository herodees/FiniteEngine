#pragma once

#include "include.hpp"

namespace fin
{

    class Surface : public std::enable_shared_from_this<Surface>
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


    class Texture2D : public std::enable_shared_from_this<Texture2D>
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
        bool load_from_image(const Image& loadedSurface);
        bool load_from_image(const Image& loadedSurface, std::string_view path);
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

        Regionf get_uv(Regionf pos) const
        {
            return {pos.x1 / texture.width, pos.y1 / texture.height, pos.x2 / texture.width, pos.y2 / texture.height};
        }

        Vec2f get_uv(Vec2f pos) const
        {
            return {pos.x / texture.width, pos.y / texture.height};
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


    class SoundSource : public std::enable_shared_from_this<SoundSource>
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

    enum class GlslType
    {
        Float,
        Int,
        Bool,
        Vec2,
        Vec3,
        Vec4,
        IVec2,
        IVec3,
        IVec4,
        BVec2,
        BVec3,
        BVec4,
        Mat4,
        Sampler2D,
        SamplerCube,
        Unknown
    };

    GlslType         StringToGlslType(std::string_view str) noexcept;
    std::string_view GlslTypeToString(GlslType type) noexcept;
    size_t           GlslTypeSize(GlslType type) noexcept;

    struct Uniform
    {
        GlslType    type{};
        std::string name{};
        int32_t     location{-2};
        size_t      arraySize{};
        void*       storage{};
        size_t      storage_size{};
        bool        updated{};
    };

    class Shader2D : public std::enable_shared_from_this<Shader2D>
    {
    private:
        Shader               _shader{};
        std::string          _path;
        std::string          _vs;
        std::string          _fs;
        std::vector<Uniform> _uniforms;
        std::vector<uint8_t> _storage;

        void     AllocateUniformStorage();
        Uniform* FindUniform(std::string_view name);
        void     SetUniform(Uniform* uni);

    public:
        using Ptr = std::shared_ptr<Shader2D>;

        Shader2D() = default;
        Shader2D(Shader2D&& s) noexcept;
        Shader2D(const Shader2D&) = delete;
        ~Shader2D();
        Shader2D& operator=(Shader2D&& s) noexcept;
        Shader2D& operator=(const Shader2D&) = delete;

        bool LoadFromFile(std::string_view filePath);
        bool SaveToFile(std::string_view filePath) const;
        bool LoadFromMemory(std::string_view vs, std::string_view fs, std::string_view filePath);

        const Uniform* FindUniform(std::string_view name) const;
        void           SetUniform(std::string_view name, const float* value, size_t size = 1);
        void           SetUniform(std::string_view name, const int32_t* value, size_t size = 1);
        void           SetUniform(std::string_view name, const uint32_t* value, size_t size = 1);
        void           SetUniform(std::string_view name, const Vec2f* value, size_t size = 1);
        void           SetSampler2D(std::string_view name, const int32_t* value, size_t size = 1);
        void           SetMatrix4(std::string_view name, const Matrix* value, size_t size = 1);

        std::string& GetVertexShader();
        std::string& GetFragmentShader();

        static void Bind(Shader2D* sh);
        static void Unbind(Shader2D* sh);

        const std::string& GetPath() const;

        static Ptr LoadShared(std::string_view pth);

        explicit operator bool() const;
    };


    class Sprite2D : public std::enable_shared_from_this<Sprite2D>
    {
    private:
        Texture2D::Ptr _texture;
        std::string    _path;
        Vec2f          _origin;
        Rectf          _rect;

    public:
        using Ptr = std::shared_ptr<Sprite2D>;

        Sprite2D() = default;
        Sprite2D(Sprite2D&& s) noexcept;
        Sprite2D(const Sprite2D&) = delete;
        ~Sprite2D();
        Sprite2D&          operator=(Sprite2D&& s) noexcept;
        Sprite2D&          operator=(const Sprite2D&) = delete;
        bool               LoadFromFile(std::string_view filePath);
        bool               SaveToFile(std::string_view filePath) const;
        void               ParseSprite(std::string_view content, std::string_view dir);
        const std::string& GetPath() const;
        Texture2D*         GetTexture() const;
        bool               SetTexture(std::string_view filePath);
        Regionf            GetUVRegion() const;
        Rectf              GetRect(Vec2f pos) const;
        const Rectf&       GetRect() const;
        void               SetRect(Rectf rc);
        Vec2f              GetSize() const;
        Vec2f              GetOrigin() const;
        void               SetOrigin(Vec2f o);
        bool               IsAlphaVisible(int x, int y) const;

        static Ptr LoadShared(std::string_view pth);

        static bool CreateTextureAtlas(const std::string& folderPath,
                                       const std::string& atlasName,
                                       int                maxAtlasWidth  = 2048,
                                       int                maxAtlasHeight = 2048,
                                       bool               shrink_to_fit  = true);

        explicit operator bool() const;
    };


} // namespace fin
