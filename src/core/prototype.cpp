#include "prototype.hpp"

namespace fin
{

Catalogue::Catalogue()
{
}

Catalogue::~Catalogue()
{
}

bool Catalogue::load_from_file(std::string_view pth)
{
    unload();
    _path = pth;

    if (auto* txt = LoadFileText(_path.c_str()))
    {
        _data.from_string(txt);
        UnloadFileText(txt);

        return true;
    }

    return false;
}

void Catalogue::unload()
{
    _path.clear();
    _data.clear();
}

uint32_t Catalogue::size() const
{
    return _data["items"].size();
}

const std::string &Catalogue::path() const
{
    return _path;
}

} // namespace fin
