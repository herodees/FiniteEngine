#pragma once

#include "atlas.hpp"
#include "include.hpp"
#include "utils/lquery.hpp"
#include "utils/spatial.hpp"

namespace fin
{
    class Renderer;
    class Scene;

    enum SceneObjectFlag
    {
        Marked   = 1 << 0,
        Disabled = 1 << 1,
        Hidden   = 1 << 2,
    };

    namespace Sc
    {
        constexpr std::string_view Id("$id");
        constexpr std::string_view Uid("$uid");
        constexpr std::string_view Class("$cls");
    } // namespace Sc

    class SceneObject : public lq::SpatialDatabase::Proxy
    {
        friend class Scene;
        friend class SceneFactory;

    public:
        inline static std::string_view type_id = "sco";

        SceneObject()          = default;
        virtual ~SceneObject() = default;

        virtual void update(float dt){};
        virtual void render(Renderer& dc);

        virtual void edit_render(Renderer& dc);
        virtual bool edit_update();

        virtual void save(msg::Var& ar); // Save prefab
        virtual void load(msg::Var& ar); // Load prefab

        virtual void serialize(msg::Writer& ar);  // Save to scene
        virtual void deserialize(msg::Value& ar); // Load to scene

        virtual std::string_view object_type() const;

        bool flag_get(SceneObjectFlag f) const;
        void flag_reset(SceneObjectFlag f);
        void flag_set(SceneObjectFlag f);

        bool is_disabled() const;
        void disable(bool v);

        bool is_hidden() const;
        void hide(bool v);

        Scene* scene();

        Vec2f position() const;
        void  move(Vec2f pos);
        void  move_to(Vec2f pos);

        Atlas::Pack& set_sprite(std::string_view path, std::string_view spr);
        Atlas::Pack& sprite();

        uint64_t      prefab() const;
        msg::Var&     collision();
        Line<float>   iso() const;
        Region<float> bounding_box() const;

    protected:
        uint32_t    _id{};
        uint32_t    _flag{};
        uint64_t    _prefab_uid{};
        Scene*      _scene{};
        Atlas::Pack _img;
        Line<float> _iso;
        msg::Var    _collision;
    };


    class SceneRegion
    {
        friend class Scene;

    public:
        SceneRegion();
        ~SceneRegion();

        void move(Vec2f pos);
        void move_to(Vec2f pos);

        const Region<float>& bounding_box();
        bool                 contains(Vec2f pos);
        msg::Var&            points();
        void                 change();
        int32_t              find_point(Vec2f pt, float radius = 1);
        SceneRegion&         insert_point(Vec2f pt, int32_t n = -1);
        Vec2f                get_point(int32_t n);
        void                 set_point(Vec2f pt, int32_t n);
        int32_t              get_size() const;

        void serialize(msg::Writer& ar);  // Save to scene
        void deserialize(msg::Value& ar); // Load to scene

        bool edit_update();
        void edit_render(Renderer& dc);

    protected:
        uint32_t              _id{};
        Scene*                _scene{};
        msg::Var              _region;
        mutable bool          _need_update{};
        mutable Region<float> _bounding_box;
    };


    class SceneFactory
    {
        using fact_t = SceneObject* (*)();

    public:
        SceneFactory();
        ~SceneFactory() = default;
        static SceneFactory& instance();

        void set_root(const std::string& startPath);

        SceneObject* create(std::string_view type) const;
        SceneObject* create(uint64_t uid) const;

        template <class T>
        SceneFactory& factory(std::string_view type, std::string_view label);

        bool load_prototypes(std::string_view type);
        bool save_prototypes(std::string_view type);

        void show_workspace();
        void show_menu();
        void show_properties();
        void show_explorer();
        void center_view();

    private:
        struct ClassInfo
        {
            void                         select(int32_t n);
            bool                         is_selected() const;
            fact_t                       fn;
            std::string                  name;
            msg::Var                     items;
            int32_t                      selected{-1};
            int32_t                      active{-1};
            std::unique_ptr<SceneObject> obj{};
        };
        void        show_properties(ClassInfo& info);
        void        reset_atlas_cache();
        Atlas::Pack load_atlas(msg::Var& el);

        std::unordered_map<std::string, ClassInfo, std::string_hash, std::equal_to<>>              _factory;
        std::unordered_map<uint64_t, msg::Var>                                                     _prefab;
        std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> _cache;
        std::string                                                                                _base_folder;
        ClassInfo*                                                                                 _edit{};
        ClassInfo*                                                                                 _explore{};
        std::string                                                                                _buff;
        bool _scroll_to_center = true;
        bool _edit_origin      = false;
        bool _edit_collision   = false;
    };


    template <class T>
    inline SceneFactory& SceneFactory::factory(std::string_view type, std::string_view label)
    {
        ClassInfo& nfo = _factory[std::string(type)];
        nfo.fn         = []() { return new T(); };
        nfo.name       = label;
        nfo.obj.reset(nfo.fn());
        return *this;
    }

    inline std::string_view SceneObject::object_type() const
    {
        return type_id;
    }

} // namespace fin
