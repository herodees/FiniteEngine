#include "shared_resource.hpp"
#include "atlas.hpp"
#include <imstb_rectpack.h>
#include <rlgl.h>
#include <utils/svstream.hpp>
#include <utils/ini.hpp>

namespace fs = std::filesystem;

namespace fin
{
    struct SharedResource
    {
        Shader2D*                                                                                      _active_shader{};
        std::string                                                                                    _path;
        std::unordered_map<std::string, std::weak_ptr<Atlas>, std::string_hash, std::equal_to<>>       _atlases;
        std::unordered_map<std::string, std::weak_ptr<Texture2D>, std::string_hash, std::equal_to<>>   _textures;
        std::unordered_map<std::string, std::weak_ptr<Surface>, std::string_hash, std::equal_to<>>     _surfaces;
        std::unordered_map<std::string, std::weak_ptr<SoundSource>, std::string_hash, std::equal_to<>> _sounds;
        std::unordered_map<std::string, std::weak_ptr<Sprite2D>, std::string_hash, std::equal_to<>>    _sprites;
        std::unordered_map<std::string, std::weak_ptr<Shader2D>, std::string_hash, std::equal_to<>>    _shaders;
    };

    static SharedResource _shared_res;
    constexpr int         ICON = 64;

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

    Shader2D::Shader2D(Shader2D&& s) noexcept
    {
        std::swap(s._shader, _shader);
        std::swap(s._path, _path);
    }

    Shader2D::~Shader2D()
    {
        if (_shader.id)
            UnloadShader(_shader);
    }

    Shader2D& Shader2D::operator=(Shader2D&& s) noexcept
    {
        std::swap(s._shader, _shader);
        std::swap(s._path, _path);
        return *this;
    }

    bool Shader2D::SaveToFile(std::string_view filePath) const
    {
        std::string str;
        str.append("#type vertex\n");
        str.append(_vs);
        str.append("#type fragment\n");
        str.append(_fs);
        return SaveFileText(std::string(filePath).c_str(), str.c_str());
    }

    std::string StripComments(const std::string_view src) noexcept
    {
        std::string out;
        const char* p              = src.data();
        const char* end            = p + src.size();
        bool        inLineComment  = false;
        bool        inBlockComment = false;

        while (p < end)
        {
            if (!inLineComment && !inBlockComment && *p == '/' && (p + 1 < end))
            {
                if (*(p + 1) == '/')
                {
                    inLineComment = true;
                    p += 2;
                    continue;
                }
                if (*(p + 1) == '*')
                {
                    inBlockComment = true;
                    p += 2;
                    continue;
                }
            }

            if (inLineComment && *p == '\n')
            {
                inLineComment = false;
            }
            else if (inBlockComment && *p == '*' && (p + 1 < end) && *(p + 1) == '/')
            {
                inBlockComment = false;
                p += 2;
                continue;
            }
            else if (!inLineComment && !inBlockComment)
            {
                out += *p;
            }
            ++p;
        }

        return out;
    }

    void parseUniforms(std::vector<Uniform>& uniforms, std::string_view source) noexcept
    {
        std::string      cleaned = StripComments(source);
        std::string_view cleanedView(cleaned);

        std::string_view_stream s(cleanedView);
        while (!s.eof())
        {
            s.skip_whitespace();

            auto kw = s.parse_identifier();
            if (kw != "uniform")
            {
                s.skip_until(';');
                continue;
            }

            auto     typeStr = s.parse_identifier();
            GlslType type    = StringToGlslType(typeStr);
            if (type == GlslType::Unknown)
            {
                s.skip_until(';');
                continue;
            }

            while (!s.eof())
            {
                s.skip_whitespace();
                auto name = s.parse_identifier();
                if (name.empty())
                    break;

                int arraySize = 1;
                s.skip_whitespace();
                if (s.expect('['))
                {
                    arraySize = s.parse_integer();
                    s.expect(']');
                }

                auto& un     = uniforms.emplace_back();
                un.type      = type;
                un.name      = name;
                un.arraySize = size_t(arraySize);

                s.skip_whitespace();
                if (s.expect(','))
                {
                    continue; // more names
                }
                else if (s.expect(';'))
                {
                    break; // end of declaration
                }
                else
                {
                    s.skip_until(';');
                    break;
                }
            }
        }
    }

    bool Shader2D::LoadFromFile(std::string_view filePath)
    {
        _path = filePath;

        // Unload existing shader if it exists
        if (_shader.id)
        {
            UnloadShader(_shader);
            _shader = {0};
        }

        // Load the entire file text
        char* fileText = LoadFileText(_path.c_str());
        if (!fileText)
        {
            return false;
        }

        // Find vertex and fragment shader sections
        const char* vertexMarker   = "#type vertex";
        const char* fragmentMarker = "#type fragment";

        char* vertexStart   = strstr(fileText, vertexMarker);
        char* fragmentStart = strstr(fileText, fragmentMarker);

        if (!vertexStart || !fragmentStart)
        {
            UnloadFileText(fileText);
            return false; // Missing one of the shader types
        }
        char* vertexEnd = fragmentStart;

        // Move pointers to the start of the actual shader code
        vertexStart += strlen(vertexMarker);
        while (*vertexStart == '\r' || *vertexStart == '\n')
            vertexStart++;

        fragmentStart += strlen(fragmentMarker);
        while (*fragmentStart == '\r' || *fragmentStart == '\n')
            fragmentStart++;

        // Extract shader code (assume fragment comes after vertex)
        char* fragmentEnd = fileText + strlen(fileText);

        // Null-terminate the shader strings
        *vertexEnd             = '\0';
        char* vertexShaderCode = vertexStart;

        *fragmentEnd             = '\0';
        char* fragmentShaderCode = fragmentStart;

        _vs.assign(vertexShaderCode);
        _fs.assign(fragmentShaderCode);
        // Load shader from memory
        _shader = LoadShaderFromMemory(vertexShaderCode, fragmentShaderCode);
        AllocateUniformStorage();

        // Cleanup
        UnloadFileText(fileText);

        return _shader.id != 0;
    }

    bool Shader2D::LoadFromMemory(std::string_view vs, std::string_view fs, std::string_view filePath)
    {
        _path = filePath;

        // Unload existing shader if it exists
        if (_shader.id)
        {
            UnloadShader(_shader);
            _shader = {0};
        }

        _vs.assign(vs);
        _fs.assign(fs);

        _shader = LoadShaderFromMemory(_vs.c_str(), _fs.c_str());
        AllocateUniformStorage();

        return _shader.id != 0;
    }

    void Shader2D::AllocateUniformStorage()
    {
        _uniforms.clear();
        if (!_shader.id)
            return;

        parseUniforms(_uniforms, _vs);
        parseUniforms(_uniforms, _fs);

        for (size_t n = 0; n < _uniforms.size(); ++n)
        {
            for (size_t i = n + 1; i < _uniforms.size(); ++i)
            {
                if (_uniforms[n].name == _uniforms[i].name)
                {
                    _uniforms[i].name.clear();
                }
            }
        }

        for (int32_t n = int32_t(_uniforms.size()) - 1; n >= 0; --n)
        {
            if (_uniforms[n].name.empty())
            {
                _uniforms[n] = _uniforms.back();
                _uniforms.pop_back();
            }
        }

        size_t offset = 0;
        for (auto& u : _uniforms)
        {
            size_t type_size  = GlslTypeSize(u.type);
            size_t total_size = type_size * u.arraySize; // support arrays

            // Optional: align to 16 bytes (for UBO-style uniform handling)
            size_t alignment      = 16;
            size_t aligned_offset = (offset + alignment - 1) & ~(alignment - 1);

            u.storage      = reinterpret_cast<void*>(static_cast<size_t>(aligned_offset));
            u.storage_size = static_cast<size_t>(total_size);

            if (_storage.size() < aligned_offset + total_size)
                _storage.resize(aligned_offset + total_size);

            offset = aligned_offset + total_size;
        }

        for (auto& u : _uniforms)
            u.storage = _storage.data() + reinterpret_cast<size_t>(u.storage);
    }

    Uniform* Shader2D::FindUniform(std::string_view name)
    {
        for (auto& un : _uniforms)
            if (un.name == name)
                return &un;
        return nullptr;
    }

    void Shader2D::SetUniform(Uniform* uni)
    {
        if (uni->updated)
            return;

        if (_shared_res._active_shader != this)
            return;

        if (uni->location == -2)
        {
            uni->location = rlGetLocationUniform(_shader.id, uni->name.c_str());
            if (uni->location < 0)
            {
                TraceLog(LOG_ERROR, "Undefined uniform location: '%s'", uni->name.c_str());
                return;
            }
        }
        uni->updated = true;

        switch (uni->type)
        {
            case GlslType::Float:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_FLOAT, uni->arraySize);
                return;
            case GlslType::Int:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_INT, uni->arraySize);
                return;
            case GlslType::Bool:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_INT, uni->arraySize);
                return; // GLSL bools are 4 bytes on GPU
            case GlslType::Vec2:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_VEC2, uni->arraySize);
                return;
            case GlslType::Vec3:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_VEC3, uni->arraySize);
                return;
            case GlslType::Vec4:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_VEC4, uni->arraySize);
                return;
            case GlslType::IVec2:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC2, uni->arraySize);
                return;
            case GlslType::IVec3:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC3, uni->arraySize);
                return;
            case GlslType::IVec4:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC4, uni->arraySize);
                return;
            case GlslType::BVec2:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC2, uni->arraySize);
                return;
            case GlslType::BVec3:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC3, uni->arraySize);
                return;
            case GlslType::BVec4:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_IVEC4, uni->arraySize);
                return;
            case GlslType::Mat4:
                rlSetUniformMatrices(uni->location, (Matrix*)uni->storage, uni->arraySize);
                return;
            case GlslType::Sampler2D:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_UINT, uni->arraySize);
                return; // treated as int
            case GlslType::SamplerCube:
                rlSetUniform(uni->location, uni->storage, RL_SHADER_UNIFORM_UINT, uni->arraySize);
                return;
            default:
                return;
        }
    }

    const Uniform* Shader2D::FindUniform(std::string_view name) const
    {
        for (auto& un : _uniforms)
            if (un.name == name)
                return &un;
        return nullptr;
    }

    void Shader2D::SetUniform(std::string_view name, const float* value, size_t size)
    {
        if (auto* un =  FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (float*)un->storage);
            SetUniform(un);
        }
    }

    void Shader2D::SetUniform(std::string_view name, const int32_t* value, size_t size)
    {
        if (auto* un = FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (int32_t*)un->storage);
            SetUniform(un);
        }
    }

    void Shader2D::SetUniform(std::string_view name, const uint32_t* value, size_t size)
    {
        if (auto* un = FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (uint32_t*)un->storage);
            SetUniform(un);
        }
    }

    void Shader2D::SetUniform(std::string_view name, const Vec2f* value, size_t size)
    {
        if (auto* un = FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (Vec2f*)un->storage);
            SetUniform(un);
        }
    }

    void Shader2D::SetSampler2D(std::string_view name, const int32_t* value, size_t size)
    {
        if (auto* un = FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (int32_t*)un->storage);
            SetUniform(un);
        }
    }

    void Shader2D::SetMatrix4(std::string_view name, const Matrix* value, size_t size)
    {
        if (auto* un = FindUniform(name))
        {
            un->updated = false;
            std::copy(value, value + size, (Matrix*)un->storage);
            SetUniform(un);
        }
    }

    std::string& Shader2D::GetVertexShader()
    {
        return _vs;
    }

    std::string& Shader2D::GetFragmentShader()
    {
        return _fs;
    }

    void Shader2D::Bind(Shader2D* sh)
    {
        if (sh)
        {
            _shared_res._active_shader = sh;
            BeginShaderMode(sh->_shader);
            for (auto& un : sh->_uniforms)
            {
                sh->SetUniform(&un);
            }
        }
        else
        {
            Unbind(_shared_res._active_shader);
        }
    }

    void Shader2D::Unbind(Shader2D* sh)
    {
        EndShaderMode();
        _shared_res._active_shader = nullptr;
    }

    Shader2D::Ptr Shader2D::LoadShared(std::string_view pth)
    {
        auto it = _shared_res._shaders.find(pth);
        if (it != _shared_res._shaders.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                               = std::make_shared<Shader2D>();
        _shared_res._shaders[std::string(pth)] = ptr;
        ptr->LoadFromFile(pth);
        return ptr;
    }

    const std::string& Shader2D::GetPath() const
    {
        return _path;
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

    bool Texture2D::load_from_image(const Image& loadedSurface)
    {
        clear(); // Free existing texture if any

        texture = LoadTextureFromImage(loadedSurface);
        generate_alpha_mask(loadedSurface, 0.5f);

        if (!texture.id)
        {
            return false;
        }

        return true;
    }

    bool Texture2D::load_from_image(const Image& loadedSurface, std::string_view path)
    {
        if (load_from_image(loadedSurface))
        {
            Texture2D::path = path;
            return true;
        }
        return false;
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

        if (pth == std::string_view("@empty"))
        {
            Image img = GenImageColor(ICON, ICON, BLANK);
            ImageDrawRectangleLines(&img, {0, 0, ICON, ICON}, 2, MAGENTA);
            ImageDrawLine(&img, 0, 0, ICON, ICON, MAGENTA);
            ImageDrawLine(&img, ICON, 0, 0, ICON, MAGENTA);
            ImageDrawRectangle(&img, 2, ICON / 2 - 5, ICON-4, 12, {255, 0, 255, 128});
            ImageDrawText(&img, "EMPTY", (ICON - 35) / 2, ICON / 2 - 3, 10, WHITE);
            ptr->load_from_image(img, "@empty");
            UnloadImage(img);
        }
        else
        {
            ptr->load_from_file(pth);
        }
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


    Sprite2D::Sprite2D(Sprite2D&& s) noexcept
    {
        std::swap(_texture, s._texture);
        std::swap(_rect, s._rect);
    }

    Sprite2D::~Sprite2D()
    {
    }

    Sprite2D& Sprite2D::operator=(Sprite2D&& s) noexcept
    {
        std::swap(_texture, s._texture);
        std::swap(_rect, s._rect);
        return *this;
    }

    bool Sprite2D::LoadFromFile(std::string_view filePath)
    {
        _path = filePath;
        int size = 0;
        if (auto* txt = LoadFileData(_path.c_str(), &size))
        {
            filePath.rfind('/') != std::string_view::npos
                ? filePath = filePath.substr(0, filePath.rfind('/'))
                : filePath = ".";
            ParseSprite({(const char*)txt, (size_t)size}, filePath);
            UnloadFileData(txt);
        }
        else
        {
            _texture = Texture2D::load_shared(std::string_view("@empty"));
            _rect    = {0, 0, ICON, ICON};
            _origin  = {};
        }
        return !!_texture;
    }

    bool Sprite2D::SaveToFile(std::string_view filePath) const
    {
        std::string data;
        data.append("[sprite]\n");
        data.append("x=").append(std::to_string((int)_rect.x)).append("\n");
        data.append("y=").append(std::to_string((int)_rect.y)).append("\n");
        data.append("width=").append(std::to_string((int)_rect.width)).append("\n");
        data.append("height=").append(std::to_string((int)_rect.height)).append("\n");
        data.append("ox=").append(std::to_string(_origin.x)).append("\n");
        data.append("oy=").append(std::to_string(_origin.y)).append("\n");
        data.append("src=").append(_texture ? GetFileName(_texture.get()->get_path().c_str()) : "").append("\n");
        return SaveFileText(std::string(filePath).c_str(), data.c_str());
    }

    // Helper function to trim whitespace from a string_view
    std::string_view trim_whitespace(std::string_view str)
    {
        const char* whitespace = " \t\r\n";
        size_t      start      = str.find_first_not_of(whitespace);
        if (start == std::string_view::npos)
        {
            return "";
        }
        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    void Sprite2D::ParseSprite(std::string_view content, std::string_view dir)
    {
        std::ini_config cfg;
        if (cfg.parse_inplace(content))
        {
            _rect.x = cfg.get_section("sprite", "x", 0);
            _rect.y = cfg.get_section("sprite", "y", 0);
            _rect.width = cfg.get_section("sprite", "width", 0);
            _rect.height = cfg.get_section("sprite", "height", 0);
            _origin.x    = cfg.get_section("sprite", "ox", 0.f);
            _origin.y    = cfg.get_section("sprite", "oy", 0.f);

            std::string base(dir);
            base.push_back('/');
            base.append(cfg.get_section("sprite", "src", std::string_view("")));
            _texture = Texture2D::load_shared(base);
        }

        if (!_texture)
        {
            _texture = Texture2D::load_shared(std::string_view("@empty"));
            _rect    = {0, 0, ICON, ICON};
            _origin  = {};
        }
    }

    const std::string& Sprite2D::GetPath() const
    {
        return _path;
    }

    Texture2D* Sprite2D::GetTexture() const
    {
        return _texture.get();
    }

    bool Sprite2D::SetTexture(std::string_view filePath)
    {
        _texture = Texture2D::load_shared(filePath);
        return !!_texture;
    }

    Regionf Sprite2D::GetUVRegion() const
    {
        return GetTexture()->get_uv(_rect.region());
    }

    Rectf Sprite2D::GetRect(Vec2f pos) const
    {
        return Rectf(pos.x - _origin.x, pos.y - _origin.y, _rect.width, _rect.height);
    }

    const Rectf& Sprite2D::GetRect() const
    {
        return _rect;
    }

    void Sprite2D::SetRect(Rectf rc)
    {
        _rect = rc;
    }

    Vec2f Sprite2D::GetSize() const
    {
        return _rect.size();
    }

    Vec2f Sprite2D::GetOrigin() const
    {
        return _origin;
    }

    void Sprite2D::SetOrigin(Vec2f o)
    {
        _origin = o;
    }

    bool Sprite2D::IsAlphaVisible(int x, int y) const
    {
        if (_texture)
        {
            return _texture->is_alpha_visible(uint32_t(x + _rect.x), uint32_t(y + _rect.y));
        }
        return false;
    }

    Sprite2D::Ptr Sprite2D::LoadShared(std::string_view pth)
    {
        auto it = _shared_res._sprites.find(pth);
        if (it != _shared_res._sprites.end() && !it->second.expired())
            return it->second.lock();

        auto ptr                               = std::make_shared<Sprite2D>();
        _shared_res._sprites[std::string(pth)] = ptr;
        ptr->LoadFromFile(pth);
        return ptr;
    }

    bool Sprite2D::CreateTextureAtlas(const std::string& folderPath,
                            const std::string& atlasName,
                            int                maxAtlasWidth,
                            int                maxAtlasHeight,
                            bool               shrink_to_fit)
    {
        // Collect all image files in the folder
        std::vector<std::string> imageFiles;
        for (const auto& entry : fs::directory_iterator(folderPath))
        {
            if (entry.is_regular_file())
            {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".gif")
                {
                    if (entry.path().stem().string() != atlasName)
                    {
                        imageFiles.push_back(entry.path().string());
                    }
                }
            }
        }

        if (imageFiles.empty())
        {
            TraceLog(LOG_WARNING, "No images found in folder to pack");
            return false;
        }

        struct PackedImage
        {
            std::string srcPath;
            ::Rectangle rect;
        };

        // Load all images and collect their sizes
        std::vector<Image>       images;
        std::vector<stbrp_rect>  rects;
        std::vector<PackedImage> packedImages;

        for (const auto& file : imageFiles)
        {
            Image img = LoadImage(file.c_str());
            if (img.data == nullptr)
            {
                TraceLog(LOG_WARNING, TextFormat("Failed to load image: %s", file.c_str()));
                continue;
            }

            stbrp_rect rect;
            rect.id = rects.size();
            rect.w  = img.width;
            rect.h  = img.height;
            rect.x = rect.y = 0;
            rect.was_packed = 0;

            rects.push_back(rect);
            images.push_back(img);
            packedImages.push_back({file, {0, 0, (float)img.width, (float)img.height}});
        }

        // Determine optimal atlas size if shrink_to_fit is enabled
        int atlasWidth  = maxAtlasWidth;
        int atlasHeight = maxAtlasHeight;

        if (shrink_to_fit)
        {
            // Calculate total area and maximum dimensions
            int totalArea = 0;
            int maxWidth  = 0;
            int maxHeight = 0;

            for (const auto& rect : rects)
            {
                totalArea += rect.w * rect.h;
                maxWidth  = std::max(maxWidth, rect.w);
                maxHeight = std::max(maxHeight, rect.h);
            }

            // Start with reasonable initial size
            atlasWidth  = std::min(maxAtlasWidth, NextPowerOfTwo((int)ceil(sqrt(totalArea * 1.2))));
            atlasHeight = std::min(maxAtlasHeight, NextPowerOfTwo((int)ceil(sqrt(totalArea * 1.2))));

            // Ensure the atlas can fit the largest image
            atlasWidth  = std::max(atlasWidth, maxWidth);
            atlasHeight = std::max(atlasHeight, maxHeight);

            // Try packing with progressively smaller sizes
            bool packed = false;
            while (!packed && (atlasWidth >= maxWidth && atlasHeight >= maxHeight))
            {
                stbrp_context           context;
                std::vector<stbrp_node> nodes(atlasWidth);
                stbrp_init_target(&context, atlasWidth, atlasHeight, nodes.data(), nodes.size());

                // Make a copy of rects to test packing
                std::vector<stbrp_rect> testRects = rects;
                packed                            = stbrp_pack_rects(&context, testRects.data(), testRects.size()) != 0;

                if (!packed)
                {
                    // Try increasing the smaller dimension
                    if (atlasWidth <= atlasHeight && atlasWidth < maxAtlasWidth)
                    {
                        atlasWidth *= 2;
                    }
                    else if (atlasHeight < maxAtlasHeight)
                    {
                        atlasHeight *= 2;
                    }
                    else
                    {
                        break; // Can't fit even at max size
                    }
                }
            }

            if (!packed)
            {
                TraceLog(LOG_WARNING, "Couldn't find optimal size, using maximum dimensions");
                atlasWidth  = maxAtlasWidth;
                atlasHeight = maxAtlasHeight;
            }
        }

        TraceLog(LOG_INFO, TextFormat("Using atlas size: %dx%d", atlasWidth, atlasHeight));

        // Initialize rectangle packer with final size
        stbrp_context           context;
        std::vector<stbrp_node> nodes(atlasWidth);
        stbrp_init_target(&context, atlasWidth, atlasHeight, nodes.data(), nodes.size());

        // Pack the rectangles
        if (stbrp_pack_rects(&context, rects.data(), rects.size()) == 0)
        {
            TraceLog(LOG_ERROR, "Failed to pack rectangles into atlas");
            for (auto& img : images)
                UnloadImage(img);
            return false;
        }

        // Create atlas image
        Image atlasImg = GenImageColor(atlasWidth, atlasHeight, BLANK);

        // Copy packed images to atlas
        for (size_t i = 0; i < rects.size(); i++)
        {
            if (rects[i].was_packed)
            {
                ::Rectangle srcRec = {0, 0, (float)images[i].width, (float)images[i].height};
                ::Rectangle dstRec = {(float)rects[i].x, (float)rects[i].y, (float)rects[i].w, (float)rects[i].h};

                ImageDraw(&atlasImg, images[i], srcRec, dstRec, WHITE);
                packedImages[i].rect = dstRec;
            }
            else
            {
                TraceLog(LOG_WARNING, TextFormat("Failed to pack image: %s", packedImages[i].srcPath.c_str()));
            }
        }

        // Save the atlas image
        std::string atlasPath = folderPath + atlasName + ".qoi";
        ExportImage(atlasImg, atlasPath.c_str());
        SaveFileText((folderPath + ".metadata").c_str(), "type=atlas");

        // Save sprite metadata files
        for (const auto& packedImg : packedImages)
        {
            if (packedImg.rect.width > 0 && packedImg.rect.height > 0)
            {
                Sprite2D::Ptr old_data;
                std::string spriteFilePath = packedImg.srcPath;
                spriteFilePath.resize(spriteFilePath.find_last_of('.'));
                spriteFilePath.append(".sprite");
                std::string data;
                data.append("[sprite]\n");
                data.append("x=").append(std::to_string((int)packedImg.rect.x)).append("\n");
                data.append("y=").append(std::to_string((int)packedImg.rect.y)).append("\n");
                data.append("width=").append(std::to_string((int)packedImg.rect.width)).append("\n");
                data.append("height=").append(std::to_string((int)packedImg.rect.height)).append("\n");
                if (FileExists(spriteFilePath.c_str()))
                {
                    if (old_data = Sprite2D::LoadShared(spriteFilePath))
                    {
                        data.append("ox=").append(std::to_string(old_data->GetOrigin().x)).append("\n");
                        data.append("oy=").append(std::to_string(old_data->GetOrigin().y)).append("\n");
                    }
                }
                data.append("src=.qoi\n");
                SaveFileText(spriteFilePath.c_str(), data.c_str());
                if (old_data)
                {
                    auto txt = Texture2D::load_shared(folderPath + ".qoi");
                    txt->load_from_file(folderPath + ".qoi");
                    old_data->SetRect({packedImg.rect.x, packedImg.rect.y, packedImg.rect.width, packedImg.rect.height});
                }
            }
        }

        // Cleanup
        for (auto& img : images)
            UnloadImage(img);
        UnloadImage(atlasImg);

        TraceLog(LOG_INFO,
                 TextFormat("Successfully created texture atlas: %s (%dx%d)", atlasPath.c_str(), atlasWidth, atlasHeight));
        return true;
    }

    Sprite2D::operator bool() const
    {
        return !!_texture;
    }

    size_t GlslTypeSize(GlslType type) noexcept
    {
        switch (type)
        {
            case GlslType::Float:
                return 4;
            case GlslType::Int:
                return 4;
            case GlslType::Bool:
                return 4; // GLSL bools are 4 bytes on GPU
            case GlslType::Vec2:
                return 4 * 2;
            case GlslType::Vec3:
                return 4 * 3;
            case GlslType::Vec4:
                return 4 * 4;
            case GlslType::IVec2:
                return 4 * 2;
            case GlslType::IVec3:
                return 4 * 3;
            case GlslType::IVec4:
                return 4 * 4;
            case GlslType::BVec2:
                return 4 * 2;
            case GlslType::BVec3:
                return 4 * 3;
            case GlslType::BVec4:
                return 4 * 4;
            case GlslType::Mat4:
                return 4 * 4 * 4;
            case GlslType::Sampler2D:
                return 4; // treated as int
            case GlslType::SamplerCube:
                return 4;
            default:
                return 0;
        }
    }

    GlslType StringToGlslType(std::string_view str) noexcept
    {
        if (str == "float")
            return GlslType::Float;
        if (str == "int")
            return GlslType::Int;
        if (str == "bool")
            return GlslType::Bool;
        if (str == "vec2")
            return GlslType::Vec2;
        if (str == "vec3")
            return GlslType::Vec3;
        if (str == "vec4")
            return GlslType::Vec4;
        if (str == "ivec2")
            return GlslType::IVec2;
        if (str == "ivec3")
            return GlslType::IVec3;
        if (str == "ivec4")
            return GlslType::IVec4;
        if (str == "bvec2")
            return GlslType::BVec2;
        if (str == "bvec3")
            return GlslType::BVec3;
        if (str == "bvec4")
            return GlslType::BVec4;
        if (str == "mat2")
            return GlslType::Mat4;
        if (str == "sampler2D")
            return GlslType::Sampler2D;
        if (str == "samplerCube")
            return GlslType::SamplerCube;
        return GlslType::Unknown;
    }

    std::string_view GlslTypeToString(GlslType type) noexcept
    {
        switch (type)
        {
            case GlslType::Float:
                return "float";
            case GlslType::Int:
                return "int";
            case GlslType::Bool:
                return "bool";
            case GlslType::Vec2:
                return "vec2";
            case GlslType::Vec3:
                return "vec3";
            case GlslType::Vec4:
                return "vec4";
            case GlslType::IVec2:
                return "ivec2";
            case GlslType::IVec3:
                return "ivec3";
            case GlslType::IVec4:
                return "ivec4";
            case GlslType::BVec2:
                return "bvec2";
            case GlslType::BVec3:
                return "bvec3";
            case GlslType::BVec4:
                return "bvec4";
            case GlslType::Mat4:
                return "mat4";
            case GlslType::Sampler2D:
                return "sampler2D";
            case GlslType::SamplerCube:
                return "samplerCube";
            default:
                return "unknown";
        }
    }


} // namespace fin
