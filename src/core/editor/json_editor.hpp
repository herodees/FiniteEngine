#pragma once

#include "include.hpp"
#include "imgui_control.hpp"

namespace fin
{
class JsonEdit;
struct JsonVal
{
    msg::Var get_item()
    {
        if (k.data())
            return p.get_item(k);
        if (n == 0xffffffff)
            return p;
        return p.get_item(n);
    }

    void set_item(const msg::Var &v)
    {
        if (k.data())
            p.set_item(k, v);
        else if (n == 0xffffffff)
            p = v;
        else
            p.set_item(n, v);
    }

    msg::Var p;
    std::string_view k;
    uint32_t n = 0;
};

class JsonType
{
public:
    JsonType(std::string_view id);
    virtual ~JsonType() = default;

    virtual bool edit(msg::Var &sch, JsonVal &value, std::string_view key) = 0;

    std::string _name;
    JsonType *_next{};
    JsonEdit *_edit{};
};

class JsonEdit
{
public:
    JsonEdit();
    ~JsonEdit();

    bool show(msg::Var &data, const char* title);

    void schema(const char *sch);
    void schema(msg::Var &sch);

    template <class T>
    JsonEdit &add();

    void next_open();
    bool show_schema(msg::Var &sch, JsonVal &value, std::string_view key);
    bool show_object(msg::Var &sch, JsonVal &value, std::string_view key);
    bool show_array(msg::Var &sch, JsonVal &value, std::string_view key);

protected:
    msg::Var *_root{};
    JsonType *_types{};
    msg::Var _schema;
    std::string _buffer;
    bool _next_open = false;
};

template <class T>
inline JsonEdit &JsonEdit::add()
{
    auto ptr = new T();
    ptr->_next = _types;
    ptr->_edit = this;
    _types = ptr;
    return *this;
}

} // namespace fin
