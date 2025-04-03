#pragma once

#include "include.hpp"
#include "utils/imgui_utils.hpp"

namespace fin
{

class JsonEdit
{
public:
    JsonEdit() = default;
    virtual ~JsonEdit() = default;

    bool show(msg::Var &data);
    void schema(const char *sch);
    void schema(msg::Var &sch);

protected:
    struct Val
    {
        msg::Var get_item()
        {
            if (k.data())
                return p.get_item(k);
            return p.get_item(n);
        }

        void set_item(const msg::Var &v)
        {
            if (k.data())
                 p.set_item(k,v);
            else
                p.set_item(n,v);
        }

        msg::Var p;
        std::string_view k;
        uint32_t n;
    };

    bool show_array(msg::Var &sch, msg::Var &dat);
    bool show_object(msg::Var &sch, msg::Var &dat);
    bool show_schema(msg::Var &sch, Val &value, std::string_view key);

    msg::Var _schema;
    std::string _buffer;
};

} // namespace fin
