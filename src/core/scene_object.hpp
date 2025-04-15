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

    public:
        inline static std::string_view type_id = "scno";

        SceneObject()          = default;
        virtual ~SceneObject() = default;

        virtual void             update(float dt) {};
        virtual void             render(Renderer& dc);
        virtual void             render_edit(Renderer& dc);
        virtual bool             edit();
        virtual void             serialize(msg::Writer& ar);
        virtual void             deserialize(msg::Value& ar);
        virtual void             get_iso(Region<float>& bbox, Line<float>& origin) {};
        virtual std::string_view object_type()
        {
            return type_id;
        };

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

        void attach(Scene* scene);
        void detach();

        Atlas::Pack&  set_sprite(std::string_view path, std::string_view spr);
        Atlas::Pack&  sprite();
        Line<float>   iso() const;
        Region<float> bounding_box() const;

        virtual void save(msg::Var& ar);
        virtual void load(msg::Var& ar);

        static SceneObject* create(msg::Var& ar);

    protected:
        uint32_t    _id{};
        uint32_t    _flag{};
        Scene*      _scene{};
        Atlas::Pack _img;
        Line<float> _iso;
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

    private:
        struct ClassInfo
        {
            void                         select(int32_t n);
            bool                         is_selected() const;
            fact_t                       fn;
            std::string                  name;
            msg::Var                     items;
            int32_t                      selected{-1};
            std::unique_ptr<SceneObject> obj{};
        };
        void show_properties(ClassInfo& info);

        std::unordered_map<std::string, ClassInfo, std::string_hash, std::equal_to<>> _factory;
        std::unordered_map<uint64_t, msg::Var>                                        _prefab;
        std::string                                                                   _base_folder;
        ClassInfo*                                                                    _edit{};
        std::string                                                                   _buff;
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

} // namespace fin
