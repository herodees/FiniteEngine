#include "gui_layer.hpp"

#include <charconv>
#include <span>
#include "utils/mustache.hpp"

namespace fin
{
    int text_char_from_utf8(unsigned int* out_char, const char* in_text, const char* in_text_end)
    {
        static const char     lengths[32] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                             0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0};
        static const int      masks[]     = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
        static const uint32_t mins[]      = {0x400000, 0, 0x80, 0x800, 0x10000};
        static const int      shiftc[]    = {0, 18, 12, 6, 0};
        static const int      shifte[]    = {0, 6, 4, 2, 0};
        int                   len         = lengths[*(const unsigned char*)in_text >> 3];
        int                   wanted      = len + (len ? 0 : 1);

        if (in_text_end == nullptr)
            in_text_end = in_text + wanted;

        unsigned char s[4];
        s[0] = in_text + 0 < in_text_end ? in_text[0] : 0;
        s[1] = in_text + 1 < in_text_end ? in_text[1] : 0;
        s[2] = in_text + 2 < in_text_end ? in_text[2] : 0;
        s[3] = in_text + 3 < in_text_end ? in_text[3] : 0;

        *out_char = (uint32_t)(s[0] & masks[len]) << 18;
        *out_char |= (uint32_t)(s[1] & 0x3f) << 12;
        *out_char |= (uint32_t)(s[2] & 0x3f) << 6;
        *out_char |= (uint32_t)(s[3] & 0x3f) << 0;
        *out_char >>= shiftc[len];

        int e = 0;
        e     = (*out_char < mins[len]) << 6;
        e |= ((*out_char >> 11) == 0x1b) << 7;
        e |= (*out_char > 0x10FFFF) << 8;
        e |= (s[1] & 0xc0) >> 2;
        e |= (s[2] & 0xc0) >> 4;
        e |= (s[3]) >> 6;
        e ^= 0x2a;
        e >>= shifte[len];

        if (e)
        {
            wanted    = std::min(wanted, !!s[0] + !!s[1] + !!s[2] + !!s[3]);
            *out_char = 0xFFFD;
        }
        return wanted;
    }


    class SimpleXmlParser
    {
    public:
        void on_begin_tag(std::string_view name, std::span<std::pair<std::string_view, std::string_view>> attrs)
        {
            printf("  <%.*s ", (uint32_t)name.size(), name.data());
            for (auto el : attrs)
            {
                printf("  %.*s = %.*s",
                       (uint32_t)el.first.size(),
                       el.first.data(),
                       (uint32_t)el.second.size(),
                       el.second.data());
            }
            printf(">\n");
        };
        void on_end_tag(std::string_view name)
        {
            printf("  </%.*s>\n", (uint32_t)name.size(), name.data());
        };
        void on_text(std::string_view text)
        {
            printf("  %.*s\n", (uint32_t)text.size(), text.data());
        };

        void parse(std::string_view input)
        {
            const char* p   = input.data();
            const char* end = p + input.size();

            while (p < end)
            {
                if (*p == '<')
                {
                    if (p + 1 < end && p[1] == '/')
                    {
                        // End tag
                        p += 2;
                        auto name = read_name(p, end);
                        skip_until('>', p, end);
                        on_end_tag(name);
                        ++p;
                    }
                    else
                    {
                        // Begin tag
                        ++p;
                        auto name         = read_name(p, end);
                        auto attrs        = read_attrs(p, end);
                        bool self_closing = false;

                        skip_spaces(p, end);
                        if (p < end && *p == '/')
                        {
                            self_closing = true;
                            ++p; // Skip '/'
                        }
                        if (p < end && *p == '>')
                        {
                            ++p; // Skip '>'
                            on_begin_tag(name, attrs);
                            if (self_closing)
                                on_end_tag(name);
                        }
                    }
                }
                else
                {
                    // Text node
                    const char* start = p;
                    while (p < end && *p != '<')
                        ++p;
                    std::string_view text(start, p - start);
                    if (!text.empty())
                        on_text(text);
                }
            }
        }

    private:
        static void skip_spaces(const char*& p, const char* end)
        {
            while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
                ++p;
        }

        static std::string_view read_name(const char*& p, const char* end)
        {
            const char* start = p;
            while (p < end && (isalnum(*p) || *p == '-' || *p == '_'))
                ++p;
            return std::string_view(start, p - start);
        }

        static std::span<std::pair<std::string_view, std::string_view>> read_attrs(const char*& p, const char* end)
        {
            static std::pair<std::string_view, std::string_view> buffer[32]; // MAX 32 attrs
            int                                                  count = 0;

            while (p < end && *p != '>' && *p != '/')
            {
                skip_spaces(p, end);
                if (*p == '>' || *p == '/')
                    break;

                auto key = read_name(p, end);
                skip_spaces(p, end);
                if (*p != '=')
                    break;
                ++p;
                skip_spaces(p, end);

                if (*p != '"' && *p != '\'')
                    break;
                char        quote     = *p++;
                const char* val_start = p;
                while (p < end && *p != quote)
                    ++p;
                std::string_view val(val_start, p - val_start);
                ++p;

                if (count < 16)
                    buffer[count++] = {key, val};
            }

            return std::span<std::pair<std::string_view, std::string_view>>(buffer, count);
        }

        static void skip_until(char c, const char*& p, const char* end)
        {
            while (p < end && *p != c)
                ++p;
        }
    };

    struct str
    {
        str()
        {
            SimpleXmlParser parser;

            msg::Var data;
            std::string_view pattern = R"(
    <items>{{#items}}name={{name}} {{/items}}</items>
)";
            msg::Var         arr;
            msg::Var         itm;
            itm.set_item("name", "rand");
            arr.push_back(itm);
            arr.push_back(itm);
            arr.push_back(itm);
            data.set_item("items", arr);

            auto str = mustache::render(pattern, data);
            parser.parse(str);

        }
    };





    SizeValue::SizeValue(std::string_view s)
    {
        *this = parse(s);
    }

    SizeValue::SizeValue(int32_t vv, int32_t tt) : v(vv), t(tt)
    {
    }

    SizeValue SizeValue::parse(std::string_view str)
    {
        // Trim
        while (!str.empty() && std::isspace(str.front()))
            str.remove_prefix(1);
        while (!str.empty() && std::isspace(str.back()))
            str.remove_suffix(1);

        if (str == "auto")
            return SizeValue(0, Auto);

        float       f     = 0.0f;
        const char* begin = str.data();
        const char* end   = str.data() + str.size();

        std::from_chars_result result = std::from_chars(begin, end, f);
        if (result.ec != std::errc())
        {
            return SizeValue(0, Auto);
        }

        std::string_view unit(result.ptr, end - result.ptr);

        if (unit.empty() || unit == "px")
        {
            return SizeValue(static_cast<int32_t>((f)), Px);
        }
        else if (unit == "%")
        {
            return SizeValue(static_cast<int32_t>((f * 1000)), Percent);
        }
        else if (unit == "*")
        {
            return SizeValue(static_cast<int32_t>((f * 1000)), Flex);
        }
        return SizeValue(0, Auto);
    }

    float SizeValue::value(float parent) const
    {
        switch (t)
        {
            case Px:
                return static_cast<float>(v);
            case Percent:
            case Flex:
                return (v / 1000.0f) * parent;
        }
        return NAN;
    }

    float SizeValue::value() const
    {
        switch (t)
        {
            case Px:
                return static_cast<float>(v);
            case Percent:
            case Flex:
                return v / 1000.0f; // just the flex factor, actual layout logic will need to normalize it
        }
        return NAN;
    }

    bool SizeValue::is_px() const
    {
        return t == Px;
    }

    bool SizeValue::is_flex() const
    {
        return t == Flex;
    }

    bool SizeValue::is_percent() const
    {
        return t == Percent;
    }

    bool SizeValue::is_static() const
    {
        return t == Px || t == Percent;
    }

    bool SizeValue::is_auto() const
    {
        return t == Auto;
    }

    std::string SizeValue::serialize() const
    {
        char  buf[32]; // plenty for float + unit
        char* ptr = buf;

        switch (t)
        {
            case Auto:
                return "auto";

            case Px:
                return std::to_string(v) + "px";

            case Percent:
            case Flex:
            {
                float val    = v / 1000.0f;
                auto [p, ec] = std::to_chars(buf, buf + sizeof(buf), val, std::chars_format::fixed, 3);
                if (ec != std::errc())
                    return "auto"; // fallback on encoding error

                // Remove trailing zeros and dot
                while (p > buf && *(p - 1) == '0')
                    --p;
                if (p > buf && *(p - 1) == '.')
                    --p;

                *p++ = (t == Percent) ? '%' : '*';
                return std::string(buf, p);
            }
        }

        return "auto";
    }

    GuiLayer::GuiLayer()
    {
    }

    void GuiLayer::Update(float dt)
    {
        if (_set.size())
        {
            for (auto& el : _set)
            {
                GuiControl::Rect box = _box;
                if (el->parent())
                {
                    box = el->parent()->get_box();
                }
                el->update_size(box);
            }
            _set.clear();
        }
    }

    void GuiLayer::render()
    {
        _root->render(*this);
    }

    void GuiLayer::reduce_update_set()
    {
        auto* cp = _set[0].get();
        for (int n = 1; n < _set.size(); ++n)
        {
            auto* t = _set[n].get();
            if (t->belongs_to(cp, true))
                continue;
            cp = GuiControl::find_common_parent(cp, t);
        }
        _set.clear();
        _set.push_back(cp->shared_from_this());
        cp->clear_state(GuiControl::State::Ready);
    }

    void GuiLayer::add_to_update(GuiControl* b, bool rem)
    {
        if (!b)
            return;

        if (rem)
        {
            while (b && b->parent() && !b->static_layout())
                b = b->parent();
            if (!b)
                return;
        }
        else
        {
            // do not need remeasure
            return;
        }

        if (_set.size() > 32)
            reduce_update_set(); // too many independent updates, collect them together unnder the umbrella

        int32_t n;
        for (n = (int32_t)_set.size() - 1; n >= 0; --n)
        {
            auto* t = _set[n].get();
            if (b == t)
            {
                return;
            }
            if (b->belongs_to(t, true))
            {
                b->clear_state(GuiControl::State::Ready);
                return; // don't add this as it is already under the umbrella
            }
        }

        auto* bp = (b);
        bp->clear_state(GuiControl::State::Ready);

        // find parent with known layout:
        while (bp = bp->parent())
        {
            if (bp->stops_layout_propagation())
            {
                bp->clear_state(GuiControl::State::Ready);
                break;
            }
        }

        if (!bp)
            bp = b;

        _set.push_back(bp->shared_from_this());

        // check if we have children of this one in the queue
        for (n = (int32_t)_set.size() - 1; n >= 0; --n)
        {
            auto* t = _set[n].get();
            if (t->belongs_to(bp, false))
                std::erase(_set, _set[n]);
        }

    }

    void GuiLayer::on_mouse(GuiEvent evt, int x, int y, GuiMouseButtom button)
    {
    }

    void GuiLayer::on_key(GuiEvent evt, int key, GuiKeyState alts)
    {
    }

    void GuiLayer::on_size(const GuiControl::Rect& rc)
    {
        if (!(_box.x != rc.x || _box.y != rc.y || _box.w != rc.w || _box.h != rc.h))
            return;

        _set.clear();
        _root->update_size(_box);
    }

    GuiControl::GuiControl(GuiLayer* layer) : _layer(layer)
    {
    }

    void GuiControl::set_width(SizeValue v)
    {
        clear_state(State::Ready);
        _width = v;
    }

    void GuiControl::set_height(SizeValue v)
    {
        clear_state(State::Ready);
        _height = v;
    }

    const SizeValue& GuiControl::get_width() const
    {
        return _width;
    }

    const SizeValue& GuiControl::get_height() const
    {
        return _height;
    }

    void GuiControl::set_padding(int16_t top, int16_t right, int16_t bottom, int16_t left)
    {
        clear_state(State::Ready);
        _padding.left = left;
        _padding.top  = top;
        _padding.right = right;
        _padding.bottom = bottom;
    }

    void GuiControl::set_padding(int16_t top, int16_t right_left, int16_t bottom)
    {
        set_padding(top, right_left, bottom, right_left);
    }

    void GuiControl::set_padding(int16_t top_bottom, int16_t right_left)
    {
        set_padding(top_bottom, right_left, top_bottom, right_left);
    }

    void GuiControl::set_padding(int16_t all)
    {
        set_padding(all, all, all, all);
    }

    const Box<int16_t>& GuiControl::get_padding() const
    {
        return _padding;
    }

    void GuiControl::set_margin(SizeValue top, SizeValue right, SizeValue bottom, SizeValue left)
    {
        clear_state(State::Ready);
        _margin.left   = left;
        _margin.top    = top;
        _margin.right  = right;
        _margin.bottom = bottom;
    }

    void GuiControl::set_margin(SizeValue top, SizeValue right_left, SizeValue bottom)
    {
        set_margin(top, right_left, bottom, right_left);
    }

    void GuiControl::set_margin(SizeValue top_bottom, SizeValue right_left)
    {
        set_margin(top_bottom, right_left, top_bottom, right_left);
    }

    void GuiControl::set_margin(SizeValue all)
    {
        set_margin(all, all, all, all);
    }

    bool GuiControl::stops_layout_propagation() const
    {
        return !(_width.is_auto() || _height.is_auto());
    }

    bool GuiControl::static_layout() const
    {
        return _width.is_static() && _height.is_static();
    }

    bool GuiControl::belongs_to(GuiControl* pb, bool include_this)
    {
        auto* t = include_this ? this : _parent;
        while (t)
        {
            if (t == pb)
                return true;
            t = t->_parent;
        }
        return false;
    }

    const Box<SizeValue>& GuiControl::get_margin() const
    {
        return _margin;
    }

    GuiControl* GuiControl::parent() const
    {
        return _parent;
    }

    std::string_view GuiControl::text() const
    {
        return std::string_view();
    }

    void GuiControl::insert(WPtr ptr, size_t n)
    {
        if (auto sp = ptr.lock())
        {
            if (n >= _childreen.size())
            {
                _childreen.emplace_back(sp);
                clear_state(State::Ready);
            }
            else
            {
                _childreen.insert(_childreen.begin() + n, sp);
                clear_state(State::Ready);
            }
        }
    }

    GuiControl::WPtr GuiControl::get_child(size_t n)
    {
        return _childreen[n];
    }

    size_t GuiControl::size() const
    {
        return _childreen.size();
    }

    bool GuiControl::get_state(State s) const
    {
        return _state & s;
    }

    void GuiControl::set_state(State s)
    {
        _state |= s;
    }

    void GuiControl::clear_state(State s)
    {
        _state &= ~s;
    }

    bool GuiControl::is_ready() const
    {
        return get_state(State::Ready);
    }

    bool GuiControl::is_disabled() const
    {
        return get_state(State::Disabled);
    }

    bool GuiControl::is_hidden() const
    {
        return get_state(State::Hidden);
    }

    bool GuiControl::is_focused() const
    {
        return get_state(State::Focused);
    }

    const GuiControl::Rect& GuiControl::get_box() const
    {
        if (!is_ready())
        {

        }
        return _box;
    }

    void GuiControl::render(GuiLayer& layer)
    {
        for (auto& ch : _childreen)
        {
            if (ch->is_hidden())
                continue;
            ch->render(layer);
        }
    }

    float GuiControl::get_preferred_width(float width, float height) const
    {
        float      total_width = 0;
        const auto parent_w    = _parent->get_box().w;

        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            float       w     = 0;
            const auto& wdt = el->get_width();

            if (wdt.is_static())
            {
                w = wdt.value(parent_w);
            }
            else if (wdt.is_auto())
            {
                w = el->get_preferred_width(width, height);
            }

            const auto& m = el->get_margin();
            w += m.left.value(parent_w) + m.right.value(parent_w);

            total_width = std::max(w, total_width);
        }

        return total_width;
    }

    float GuiControl::get_preferred_height(float width, float height) const
    {
        float      total_height = 0;
        const auto parent_h     = _parent->get_box().h;

        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            float       h      = 0;
            const auto& hgh = el->get_height();

            if (hgh.is_static())
            {
                h = hgh.value(parent_h);
            }
            else if (hgh.is_auto())
            {
                h = el->get_preferred_height(width, height);
            }

            const auto& m = el->get_margin();
            h += m.top.value(parent_h) + m.bottom.value(parent_h);

            total_height = std::max(h, total_height);
        }

        return total_height;
    }

    GuiControl* GuiControl::find_at(float width, float height)
    {
        return nullptr;
    }

    GuiControl::Rect& GuiControl::get_rect(GuiControl* el) const
    {
        return el->_box;
    }

    uint32_t GuiControl::get_depth() const
    {
        const GuiControl* n     = this;
        uint32_t          depth = 0;
        while (n)
        {
            n = n->_parent;
            depth++;
        }
        return depth;
    }

    GuiControl* GuiControl::find_common_parent(GuiControl* b1, GuiControl* b2)
    {
        if (!b1 || !b2)
        {
            return nullptr; // One of the nodes is null, no common parent
        }

        // If the two nodes are the same, return their parent
        if (b1 == b2)
        {
            return b1->_parent;
        }
        // Calculate depths of both nodes
        uint32_t depth1 = b1->get_depth();
        uint32_t depth2 = b2->get_depth();

        // Align both nodes at the same depth
        while (depth1 > depth2)
        {
            b1 = b1->_parent;
            depth1--;
        }
        while (depth2 > depth1)
        {
            b2 = b2->_parent;
            depth2--;
        }
        // Move both nodes upwards together until they find the common parent
        while (b1 && b2 && b1 != b2)
        {
            b1 = b1->_parent;
            b2 = b2->_parent;
        }
        // Return the common parent or nullptr if none exists
        return (b1 == b2) ? b1 : nullptr;
    }

    void GuiHorizontalLayout::render(GuiLayer& layer)
    {
        auto oldclip = layer.get_clip_rect();

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(_box);
        }

        GuiControl::render(layer);

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(oldclip);
        }
    }

    void GuiHorizontalLayout::update_size(Rect& rc)
    {
        float static_size       = 0;
        float other_size        = 0;
        float flex_total_weight = 0;

        Rect parent_rc(rc.x + _padding.left,
                       rc.y + _padding.top,
                       rc.w - _padding.left - _padding.right,
                       rc.h - _padding.top - _padding.bottom);

        // First pass: compute total static size including margins, and flex weight
        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            const auto& m     = el->get_margin();
            float       w     = 0;
            const auto& width = el->get_width();
            if (width.is_static())
            {
                w = width.value(parent_rc.w);
            }
            else if (width.is_auto())
            {
                w = el->get_preferred_width(parent_rc.w, parent_rc.h);
            }
            else if (width.is_flex())
            {
                flex_total_weight += width.value();
            }

            if (m.left.is_static())
            {
                w += m.left.value(parent_rc.w);
            }
            else if (m.left.is_flex())
            {
                flex_total_weight += m.left.value();
            }

            if (m.right.is_static())
            {
                w += m.right.value(parent_rc.w);
            }
            else if (m.right.is_flex())
            {
                flex_total_weight += m.right.value();
            }

            float       h      = 0;
            const auto& height = el->get_height();
            float       mt     = m.top.value(parent_rc.h);
            float       mb     = m.bottom.value(parent_rc.h);

            if (height.is_static())
            {
                h = height.value(parent_rc.h);
            }
            else if (height.is_auto())
            {
                h = el->get_preferred_height(parent_rc.w, parent_rc.h);
            }
            else if (height.is_flex())
            {
                h = parent_rc.h;
            }

            other_size = std::max(other_size, h + mt + mb);
        }

        float remaining = std::max(0.0f, parent_rc.w - static_size);
        float x         = parent_rc.x;

        // Second pass: assign positions and sizes
        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            el->update_size(parent_rc);

            const auto& m  = el->get_margin();
            float       ml = m.left.value(parent_rc.w);
            float       mr = m.right.value(parent_rc.w);
            float       mt = m.top.value(parent_rc.h);
            float       mb = m.bottom.value(parent_rc.h);

            float       w     = 0;
            const auto& width = el->get_width();
            if (width.is_static())
            {
                w = width.value(parent_rc.w);
            }
            else if (width.is_auto())
            {
                w = el->get_preferred_width(parent_rc.w, parent_rc.h);
            }
            else if (width.is_flex())
            {
                float weight = width.value();
                w            = (flex_total_weight > 0) ? (weight / flex_total_weight) * remaining : 0;
            }

            float       h      = 0;
            const auto& height = el->get_height();
            if (height.is_static())
            {
                h = height.value(parent_rc.h);
            }
            else if (height.is_auto())
            {
                h = el->get_preferred_height(parent_rc.w, parent_rc.h);
            }
            else if (height.is_flex())
            {
                h = parent_rc.h;
            }

            // Position and size rect with margin
            get_rect(el.get()) = Rect(x + ml, parent_rc.y + mt, w, h);
            x += w + ml + mr;
        }

        set_state(State::Ready);
        rc.h = other_size;
    }


    float GuiHorizontalLayout::get_preferred_width(float width, float height) const
    {
        float      total_width = 0;

        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            float       w     = 0;
            const auto& wdt = el->get_width();

            if (wdt.is_static())
            {
                w = wdt.value(width);
            }
            else if (wdt.is_auto())
            {
                w = el->get_preferred_width(width, height);
            }

            const auto& m = el->get_margin();
            w += m.left.value(width) + m.right.value(width);

            total_width += w;
        }

        return total_width;
    }

    void GuiVerticalLayout::render(GuiLayer& layer)
    {
        auto oldclip = layer.get_clip_rect();

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(_box);
        }

        GuiControl::render(layer);

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(oldclip);
        }
    }

    void GuiVerticalLayout::update_size(Rect& rc)
    {
        float static_size       = 0;
        float other_size        = 0;
        float flex_total_height = 0;

        Rect parent_rc(rc.x + _padding.left,
                       rc.y + _padding.top,
                       rc.w - _padding.left - _padding.right,
                       rc.h - _padding.top - _padding.bottom);

        // First pass: compute total static size including margins, and flex weight
        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            const auto& m     = el->get_margin();
            float       h     = 0;
            const auto& height = el->get_width();
            if (height.is_static())
            {
                h = height.value(parent_rc.h);
            }
            else if (height.is_auto())
            {
                h = el->get_preferred_height(parent_rc.w, parent_rc.h);
            }
            else if (height.is_flex())
            {
                flex_total_height += height.value();
            }

            if (m.top.is_static())
            {
                h += m.top.value(parent_rc.h);
            }
            else if (m.top.is_flex())
            {
                flex_total_height += m.top.value();
            }

            if (m.bottom.is_static())
            {
                h += m.bottom.value(parent_rc.h);
            }
            else if (m.bottom.is_flex())
            {
                flex_total_height += m.bottom.value();
            }

            float       w      = 0;
            const auto& width = el->get_width();
            float       mt     = m.left.value(parent_rc.w);
            float       mb     = m.right.value(parent_rc.h);

            if (width.is_static())
            {
                w = width.value(parent_rc.w);
            }
            else if (width.is_auto())
            {
                w = el->get_preferred_width(parent_rc.w, parent_rc.h);
            }
            else if (width.is_flex())
            {
                w = parent_rc.w;
            }

            other_size = std::max(other_size, w + mt + mb);
        }

        float remaining = std::max(0.0f, parent_rc.h - static_size);
        float y         = parent_rc.y;

        // Second pass: assign positions and sizes
        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            el->update_size(parent_rc);

            const auto& m  = el->get_margin();
            float       ml = m.left.value(parent_rc.w);
            float       mr = m.right.value(parent_rc.w);
            float       mt = m.top.value(parent_rc.h);
            float       mb = m.bottom.value(parent_rc.h);

            float       h     = 0;
            const auto& height = el->get_height();
            if (height.is_static())
            {
                h = height.value(parent_rc.h);
            }
            else if (height.is_auto())
            {
                h = el->get_preferred_height(parent_rc.w, parent_rc.h);
            }
            else if (height.is_flex())
            {
                float hgh = height.value();
                h         = (flex_total_height > 0) ? (hgh / flex_total_height) * remaining : 0;
            }

            float       w      = 0;
            const auto& width = el->get_width();
            if (width.is_static())
            {
                w = width.value(parent_rc.w);
            }
            else if (width.is_auto())
            {
                w = el->get_preferred_height(parent_rc.w, parent_rc.h);
            }
            else if (width.is_flex())
            {
                w = parent_rc.w;
            }

            // Position and size rect with margin
            get_rect(el.get()) = Rect(parent_rc.x + ml, y + mt, w, h);
            y += y + mt + mb;
        }

        set_state(State::Ready);
        rc.h = other_size;
    }

    float GuiVerticalLayout::get_preferred_height(float width, float height) const
    {
        float      total_height = 0;

        for (auto& el : _childreen)
        {
            if (el->is_hidden())
                continue;

            float       h      = 0;
            const auto& hgh = el->get_height();

            if (hgh.is_static())
            {
                h = hgh.value(height);
            }
            else if (hgh.is_auto())
            {
                h = el->get_preferred_height(width, height);
            }

            const auto& m = el->get_margin();
            h += m.top.value(height) + m.bottom.value(height);

            total_height += h;
        }

        return total_height;
    }

    void GuiText::render(GuiLayer& layer)
    {
        if (_text.empty())
            return;

        auto oldclip = layer.get_clip_rect();

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(_box);
        }

        if (get_state(State::NoWrap))
        {
            const auto maxw = _box.x + _box.w;
            Rect rc{_box};
            for (auto& blk : _blocks)
            {
                layer.render_text(std::string_view(_text.c_str() + blk.from, blk.size), rc, _font);
                rc.x += blk.width;
                if (rc.x > maxw)
                    break;
            }
        }
        else
        {
            float total_height = 0;
            float line_width   = 0;
            Rect  rc{};

            for (const auto& blk : _blocks)
            {
                if (_text[blk.from] == '\n')
                {
                    total_height += _font.size;
                    line_width = 0;
                    continue;
                }

                if (std::isspace(_text[blk.from]))
                {
                    // Only add whitespace if not starting the line
                    if (line_width > 0)
                        line_width += blk.width;
                    continue;
                }

                if (line_width + blk.width > _box.w)
                {
                    // Wrap to next line
                    total_height += _font.size;
                    line_width = blk.width;
                }
                else
                {
                    line_width += blk.width;
                }

                rc.x = _box.x + line_width;
                rc.y = _box.y + total_height;
                rc.w = blk.width;
                rc.h = blk.height;

                layer.render_text(std::string_view(_text.c_str() + blk.from, blk.size), rc, _font);
            }
        }

        if (!get_state(State::NoClip))
        {
            layer.set_clip_rect(oldclip);
        }
    }

    void GuiText::update_size(Rect& rc)
    {
        if (_blocks.empty() && !_text.empty())
        {
            update_blocks();
        }
    }

    float GuiText::get_preferred_width(float width, float height) const
    {
        float size = 0;

        for (auto& blk : _blocks)
        {
            size += blk.width;
        }

        return size;
    }

    float GuiText::get_preferred_height(float width, float height) const
    {
        if (get_state(State::NoWrap) || width == INFINITY)
            return _font.size;

        float line_width   = 0;
        float line_height  = _font.size; // Height of one line
        float total_height = 0;

        for (const auto& blk : _blocks)
        {
            if (_text[blk.from] == '\n')
            {
                total_height += line_height;
                line_width = 0;
                continue;
            }

            if (std::isspace(_text[blk.from]))
            {
                // Only add whitespace if not starting the line
                if (line_width > 0)
                    line_width += blk.width;
                continue;
            }

            if (line_width + blk.width > width)
            {
                // Wrap to next line
                total_height += line_height;
                line_width = blk.width;
            }
            else
            {
                line_width += blk.width;
            }
        }

        // Add final line if any content left
        if (line_width > 0)
            total_height += line_height;

        return total_height;
    }

    std::string_view GuiText::text() const
    {
        return _text;
    }

    void GuiText::set_text(std::string_view txt)
    {
        _text = txt;
        _blocks.clear();
        clear_state(State::Ready);
    }

    void GuiText::update_blocks()
    {
        _blocks.clear();

        const char*              p   = _text.data();
        const char*              end = p + _text.size();
        Block                    current_word{};

        while (p < end)
        {
            unsigned int ch       = 0;
            int          consumed = text_char_from_utf8(&ch, p, end);

            // Basic HTML-style word break characters
            bool is_split = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '.' || ch == ',' || ch == ';' ||
                             ch == ':' || ch == '!' || ch == '?' || ch == '-' || ch == '(' || ch == ')');

            if (is_split)
            {
                if (current_word.size)
                {
                    _blocks.push_back(current_word);
                    current_word.from = (uint32_t)std::distance(_text.c_str(), p);
                    current_word.size = 0;
                }
                // If you want to treat punctuation as separate word:
                if (ch != ' ' && ch != '\t' && ch != '\n')
                {
                    current_word.from = (uint32_t)std::distance(_text.c_str(), p);
                    current_word.size += consumed;
                    _blocks.push_back(current_word);
                    current_word.from += current_word.size;
                    current_word.size = 0;
                }
            }
            else
            {
                current_word.size += consumed;
            }

            p += consumed;
        }

        if (current_word.size)
        {
            _blocks.push_back(current_word);
        }

        _font.calc_size_only = true;

        Rect rc;
        for (auto& blk : _blocks)
        {
            _layer->render_text(std::string_view(_text.c_str() + blk.from, blk.size), rc, _font);
            blk.width = rc.w;
            blk.height = rc.h;
        }
        _font.calc_size_only = false;
    }

} // namespace fin
