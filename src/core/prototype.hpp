#pragma once

#include "include.hpp"

namespace fin
{
    class Prototype
    {
    public:
    };


    class Catalogue : std::enable_shared_from_this<Catalogue>
    {
    public:
        Catalogue();
        ~Catalogue();

        bool load_from_file(std::string_view pth);
        void unload();

        uint32_t size() const;
        const std::string &path() const;

        static std::shared_ptr<Catalogue> load_shared(std::string_view pth);

    protected:
        std::string _path;
        mutable msg::Var _data;
    };
}
