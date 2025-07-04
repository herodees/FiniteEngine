#pragma once

#include "shared_resource.hpp"
#include "utils/matrix2d.hpp"
#include "api/msgbuff.hpp"

namespace fin
{
    class Atlas : std::enable_shared_from_this<Atlas>
    {
    public:
        using Ptr = std::shared_ptr<Atlas>;

        struct TextureRegion
        {
            const Texture* _texture;
            Rectf          _source;
            Vec2f          _origina;
            Vec2f          _originb;
        };

        struct Sprite : TextureRegion
        {
            std::string _name;
        };

        struct Vertex
        {
            Vec2f position;
            Vec2f uv;
        };

        struct Composite
        {
            std::string    _name;
            const Texture* _texture;
            Vertex*        _begin;
            Vertex*        _end;
        };

        struct Pack
        {
            Atlas::Ptr atlas;
            Sprite*    sprite{};
            bool       is_alpha_visible(uint32_t x, uint32_t y) const;
        };

        bool               load_from_file(std::string_view pth);
        void               unload();
        const std::string& get_path() const;
        uint32_t           find_sprite(std::string_view name) const;
        uint32_t           find_composite(std::string_view name) const;
        uint32_t           size() const;
        uint32_t           size_c() const;
        const Sprite&      get(uint32_t n) const;
        Sprite&            get(uint32_t n);
        const Composite&   get_c(uint32_t n) const;
        Composite&         get_c(uint32_t n);
        const Texture2D&   texture() const;

        static Atlas::Ptr load_shared(std::string_view pth);
        static Pack       load_shared(std::string_view pth, std::string_view spr);

    protected:
        bool                   Load(msg::Value ar);
        std::string            _path;
        Texture2D              _texture{};
        std::vector<Sprite>    _sprites;
        std::vector<Composite> _composites;
        std::vector<Vertex>    _mesh;
    };


    inline bool Atlas::load_from_file(std::string_view pth)
    {
        unload();
        _path = pth;

        auto* txt = LoadFileText(_path.c_str());
        if (!txt)
            return false;

        msg::Pack doc;
        auto      r = doc.from_string(txt);
        UnloadFileText(txt);

        if (msg::VarError::ok != r)
            return false;

        return Load(doc.get());
    }

    inline void Atlas::unload()
    {
        _sprites.clear();
        _texture.clear();
    }

    inline const std::string& Atlas::get_path() const
    {
        return _path;
    }

    inline bool Atlas::Load(msg::Value ar)
    {
        auto items      = ar["items"];
        auto composites = ar["composites"];
        auto txture     = ar["texture"];
        auto meta       = ar["metadata"];

        _sprites.clear();
        _sprites.reserve(items.size());
        _composites.clear();
        _composites.reserve(composites.size());
        _mesh.clear();

        if (txture.is_object())
        {
            if (txture["file"].is_string())
            {
                std::string pth(GetPrevDirectoryPath(_path.c_str()));
                pth.append("/").append(txture["file"].str());
                _texture.load_from_file(pth);
            }
        }

        auto* txt      = texture().get_texture();
        auto  txt_size = texture().get_size();

        for (auto& el : items.elements())
        {
            auto& spr          = _sprites.emplace_back();
            spr._name          = el["id"].str();
            spr._source.x      = (float)el["x"].get(0);
            spr._source.y      = (float)el["y"].get(0);
            spr._source.width  = (float)el["w"].get(0);
            spr._source.height = (float)el["h"].get(0);
            spr._origina.x     = (float)el["oxa"].get(0);
            spr._origina.y     = (float)el["oya"].get(0);
            spr._originb.x     = (float)el["oxb"].get(0);
            spr._originb.y     = (float)el["oyb"].get(0);
            spr._texture       = txt;
        }

        for (auto& el : composites.elements())
        {
            auto& cmp    = _composites.emplace_back();
            auto  els    = el["items"];
            cmp._name    = el["id"].str();
            cmp._texture = txt;
            cmp._begin = cmp._end = (Vertex*)_mesh.size();

            for (auto& s : els.elements())
            {
                if (auto sprid = find_sprite(s["s"].str()))
                {
                    auto&    spr = get(sprid);
                    matrix2d tr({(float)s["x"].get_number(0), (float)s["y"].get_number(0)},
                                {(float)s["sx"].get_number(1.), (float)s["sy"].get_number(1.)},
                                spr._origina,
                                (float)s["r"].get_number(0));

                    Vertex vtx[4];
                    vtx[0].uv.x     = spr._source.x / txt_size.width;
                    vtx[0].uv.y     = spr._source.y / txt_size.height;
                    vtx[2].uv.x     = spr._source.x2() / txt_size.width;
                    vtx[2].uv.y     = spr._source.y2() / txt_size.height;
                    vtx[1].uv.x     = vtx[0].uv.x;
                    vtx[1].uv.y     = vtx[2].uv.y;
                    vtx[3].uv.x     = vtx[2].uv.x;
                    vtx[3].uv.y     = vtx[0].uv.y;
                    vtx[0].position = tr.transform_point(Vec2f());
                    vtx[1].position = tr.transform_point(Vec2f(0, spr._source.height));
                    vtx[2].position = tr.transform_point(Vec2f(spr._source.width, spr._source.height));
                    vtx[2].position = tr.transform_point(Vec2f(spr._source.width, 0));
                    _mesh.push_back(vtx[0]);
                    _mesh.push_back(vtx[1]);
                    _mesh.push_back(vtx[2]);
                    _mesh.push_back(vtx[3]);
                }
                cmp._end = (Vertex*)_mesh.size();
            }
        }


        for (auto& el : _composites)
        {
            el._begin = _mesh.data() + (size_t)el._begin;
            el._end   = _mesh.data() + (size_t)el._end;
        }

        return !!_texture;
    }

    inline uint32_t Atlas::find_sprite(std::string_view name) const
    {
        for (uint32_t n = 0; n < _sprites.size(); ++n)
        {
            if (_sprites[n]._name == name)
                return n + 1;
        }
        return 0;
    }

    inline uint32_t Atlas::find_composite(std::string_view name) const
    {
        for (uint32_t n = 0; n < +_composites.size(); ++n)
        {
            if (_composites[n]._name == name)
                return n + 1;
        }
        return 0;
    }

    inline uint32_t Atlas::size() const
    {
        return uint32_t(_sprites.size());
    }

    inline uint32_t Atlas::size_c() const
    {
        return uint32_t(_composites.size());
    }

    inline const Atlas::Sprite& Atlas::get(uint32_t n) const
    {
        return _sprites[n - 1];
    }

    inline Atlas::Sprite& Atlas::get(uint32_t n)
    {
        return _sprites[n - 1];
    }

    inline const Atlas::Composite& Atlas::get_c(uint32_t n) const
    {
        return _composites[n - 1];
    }

    inline Atlas::Composite& Atlas::get_c(uint32_t n)
    {
        return _composites[n - 1];
    }

    inline const Texture2D& Atlas::texture() const
    {
        return _texture;
    }

    inline bool Atlas::Pack::is_alpha_visible(uint32_t x, uint32_t y) const
    {
        if (sprite)
        {
            return atlas->_texture.is_alpha_visible(x + sprite->_source.x, y + sprite->_source.y);
        }
        return false;
    }
} // namespace fin
