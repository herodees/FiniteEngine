#pragma once

#include <vector>
#include <string>
#include <memory>

namespace fin
{
    class GuiLayer;

    using GuiFontId = uint32_t;

    struct SizeValue
    {
        enum Type
        {
            Px, Percent, Flex, Auto
        };

        SizeValue(std::string_view s);
        SizeValue() = default;
        SizeValue(int32_t vv, int32_t tt);
        static SizeValue parse(std::string_view str);

        float            value(float parent) const;
        float            value() const;
        bool             is_px() const;
        bool             is_flex() const;
        bool             is_percent() const;
        bool             is_static() const;
        bool             is_auto() const;
        std::string      serialize() const;

        int32_t v : 30 {};
        int32_t t : 2 {};
    };

    template <typename T>
    struct Box
    {
        T left{};
        T top{};
        T right{};
        T bottom{};
    };

    struct FontInfo
    {
        GuiFontId font_id{};
        uint32_t  color{};
        uint16_t  size{};
        bool      align_left{};
        bool      align_right{};
        bool      align_top{};
        bool      align_bottom{};
        bool      calc_size_only{};
    };

    class GuiControl : public std::enable_shared_from_this<GuiControl>
    {
    public:
        using WPtr = std::weak_ptr<GuiControl>;
        using Ptr  = std::shared_ptr<GuiControl>;
        struct Rect
        {
            float x{};
            float y{};
            float w{};
            float h{};
        };

        enum State
        {
            Ready    = 0x00000001,
            Disabled = 0x00000002,
            Hidden   = 0x00000004,
            Focused  = 0x00000008,
            Hovered  = 0x00000010,

            NoClip   = 0x0200000,
            Ellipsis = 0x0400000,
            NoWrap   = 0x0800000,

            AlignTop    = 0x1000000,
            AlignBottom = 0x2000000,
            AlignMiddle = AlignTop | AlignBottom,

            AlignLeft   = 0x4000000,
            AlignRight  = 0x8000000,
            AlignCenter = AlignLeft | AlignRight,
        };

        GuiControl(GuiLayer* layer);
        virtual ~GuiControl() = default;

        void             set_width(SizeValue v);
        void             set_height(SizeValue v);
        const SizeValue& get_width() const;
        const SizeValue& get_height() const;

        void                set_padding(int16_t top, int16_t right, int16_t bottom, int16_t left);
        void                set_padding(int16_t top, int16_t right_left, int16_t bottom);
        void                set_padding(int16_t top_bottom, int16_t right_left);
        void                set_padding(int16_t all);
        const Box<int16_t>& get_padding() const;

        void                  set_margin(SizeValue top, SizeValue right, SizeValue bottom, SizeValue left);
        void                  set_margin(SizeValue top, SizeValue right_left, SizeValue bottom);
        void                  set_margin(SizeValue top_bottom, SizeValue right_left);
        void                  set_margin(SizeValue all);
        const Box<SizeValue>& get_margin() const;

        GuiControl* parent() const;
        virtual std::string_view text() const;

        void   insert(WPtr ptr, size_t n = -1);
        WPtr   get_child(size_t n);
        size_t size() const;

        bool get_state(State s) const;
        void set_state(State s);
        void clear_state(State s);

        bool is_ready() const;
        bool is_disabled() const;
        bool is_hidden() const;
        bool is_focused() const;

        const Rect& get_box() const;

        virtual void  update_size(Rect& rc) = 0;
        virtual void  render(GuiLayer& layer);
        virtual float get_preferred_width(float width, float height) const;
        virtual float get_preferred_height(float width, float height) const;

        GuiControl* find_at(float width, float height);

        bool stops_layout_propagation() const;
        bool static_layout() const;
        bool belongs_to(GuiControl* pb, bool include_this);

        uint32_t           get_depth() const;
        static GuiControl* find_common_parent(GuiControl* b1, GuiControl* b2);

    protected:
        Rect&              get_rect(GuiControl* el) const;

        mutable uint32_t _state{};
        mutable Rect     _box;
        GuiLayer*        _layer{};
        GuiControl*      _parent{};
        SizeValue        _width;
        SizeValue        _height;
        Box<SizeValue>   _margin;
        Box<int16_t>     _padding;
        std::vector<Ptr> _childreen;
    };

    class GuiHorizontalLayout : public GuiControl
    {
    public:
        void  render(GuiLayer& layer) override;
        void  update_size(Rect& rc) override;
        float get_preferred_width(float width, float height) const override;
    };

    class GuiVerticalLayout : public GuiControl
    {
    public:
        void  render(GuiLayer& layer) override;
        void  update_size(Rect& rc) override;
        float get_preferred_height(float width, float height) const override;
    };

    class GuiButton : public GuiControl
    {
    };

    class GuiText : public GuiControl
    {
        struct Block
        {
            uint32_t from;
            uint32_t size;
            uint16_t width;
            uint16_t height;
        };

    public:
        void render(GuiLayer& layer) override;
        void  update_size(Rect& rc) override;
        float get_preferred_width(float width, float height) const override;
        float get_preferred_height(float width, float height) const override;

        std::string_view text() const override;
        void             set_text(std::string_view txt);

    protected:
        void update_blocks();

        std::string        _text;
        std::vector<Block> _blocks;
        FontInfo           _font;
    };

    struct GuiLayerPlatform
    {
        virtual void             init()                                                                        = 0;
        virtual void             Deinit()                                                                      = 0;
        virtual GuiFontId        font_load(std::string_view path)                                              = 0;
        virtual void             render_text(std::string_view txt, GuiControl::Rect& rc, const FontInfo& info) = 0;
        virtual void             render_solid_rect(const GuiControl::Rect rc, uint32_t clr, uint32_t align)    = 0;
        virtual void             set_clip_rect(const GuiControl::Rect& rc)                                     = 0;
        virtual GuiControl::Rect get_clip_rect()                                                               = 0;
    };

    enum GuiEvent
    {
        MouseLeft,
        MouseRight,
        MouseCenter,
        MouseWheel,
        KeyDown,
        KeyUp,
    };

    enum GuiMouseButtom
    {
        Left, Right, Center
    };

    enum GuiKeyState
    {
        Control = 1, Alt = 2, Shift = 4
    };

    class GuiLayer : public GuiLayerPlatform
    {
    public:
        GuiLayer();
        ~GuiLayer() = default;

        void Update(float dt);
        void render();

        void add_to_update(GuiControl* b, bool rem);

        void on_mouse(GuiEvent evt, int x, int y, GuiMouseButtom button);
        void on_key(GuiEvent evt, int key, GuiKeyState alts);
        void on_size(const GuiControl::Rect& rc);

    protected:
        void reduce_update_set();

        std::vector<GuiControl::Ptr> _set;
        GuiControl::Rect             _box;
        GuiControl::Ptr              _root;
        GuiControl::Ptr              _hover_el;
        int                          _mouse_button{};
        float                        _mouse_x{};
        float                        _mouse_y{};
        int                          _key_value{};
    };
} // namespace fin
