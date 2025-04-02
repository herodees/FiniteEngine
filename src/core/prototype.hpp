#pragma once

#include "include.hpp"
#include "atlas.hpp"

namespace fin
{
struct Prototype
{
    std::string _name;
    int32_t _type{};
    std::string _atlas_path;
    std::string _sprite_path;
    std::shared_ptr<Atlas> _atlas;
    Atlas::sprite *_sprite{};
    msg::Var _params;
};


class Catalogue : std::enable_shared_from_this<Catalogue>
{
public:
    Catalogue();
    ~Catalogue();

    bool load_from_file(std::string_view pth);
    void unload();

    uint32_t size() const;
    Prototype *get(uint32_t n);
    Prototype *create();

    const std::string &path() const;

    static std::shared_ptr<Catalogue> load_shared(std::string_view pth);

protected:
    std::vector<Prototype> _items;
    std::string _path;
    mutable msg::Var _data;
};
} // namespace fin
