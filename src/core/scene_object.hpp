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
        Area     = 1 << 3,
    };

    namespace Sc
    {
        constexpr std::string_view Id("$id");
        constexpr std::string_view Name("$nme");
        constexpr std::string_view Uid("$uid");
        constexpr std::string_view Class("$cls");
        constexpr std::string_view Flag("$fl");
    } // namespace Sc


    struct ObjectBase
    {
        bool flag_get(SceneObjectFlag f) const;
        void flag_reset(SceneObjectFlag f);
        void flag_set(SceneObjectFlag f);

        bool is_disabled() const;
        void disable(bool v);

        bool is_hidden() const;
        void hide(bool v);

        bool is_named() const;
        bool is_region() const;

        uint32_t    _id{};
        uint32_t    _flag{};
        const char* _name{};
    };


    class BasicSceneObject : public ObjectBase, public lq::SpatialDatabase::Proxy
    {
    protected:
        uint64_t _prefab_uid{};

    public:
        BasicSceneObject()          = default;
        virtual ~BasicSceneObject() = default;

        virtual std::string_view object_type() const = 0;

        virtual void update(float dt)     = 0;
        virtual void render(Renderer& dc) = 0;

        virtual void edit_render(Renderer& dc, bool selected) = 0;
        virtual bool imgui_update();

        virtual void save(msg::Var& ar); // Save prefab
        virtual void load(msg::Var& ar); // Load prefab

        virtual void serialize(msg::Writer& ar);  // Save to scene
        virtual void deserialize(msg::Value& ar); // Load to scene

        virtual bool sprite_object() const;

        virtual Region<float> bounding_box() const = 0;

        Vec2f    position() const;
        uint64_t prefab() const;
    };


    class IsoSceneObject : public BasicSceneObject
    {
        friend class Scene;
        friend class SceneFactory;

    public:
        void save(msg::Var& ar) override; // Save prefab
        void load(msg::Var& ar) override; // Load prefab

        void edit_render(Renderer& dc, bool selected) override;

        Line<float> iso() const;
        msg::Var&   collision();

    protected:
        Line<float> _iso;
        msg::Var    _collision;
    };


    class SpriteSceneObject : public IsoSceneObject
    {
        friend class Scene;
        friend class SceneFactory;

    public:
        inline static std::string_view type_id = "sco";

        SpriteSceneObject()           = default;
        ~SpriteSceneObject() override = default;

        void update(float dt) override {};
        void render(Renderer& dc) override;

        void edit_render(Renderer& dc, bool selected) override;
        bool imgui_update() override;

        void save(msg::Var& ar) override; // Save prefab
        void load(msg::Var& ar) override; // Load prefab

        void serialize(msg::Writer& ar) override;  // Save to scene
        void deserialize(msg::Value& ar) override; // Load to scene

        bool sprite_object() const override;

        std::string_view object_type() const override;

        Atlas::Pack& set_sprite(std::string_view path, std::string_view spr);
        Atlas::Pack& sprite();

        Region<float> bounding_box() const override;

    protected:
        Atlas::Pack _img;
    };


    class SoundObject : public IsoSceneObject
    {
        friend class Scene;
        friend class SceneFactory;

    public:
        inline static std::string_view type_id = "sso";

        SoundObject();
        ~SoundObject() override;

        void update(float dt) override;
        void render(Renderer& dc) override;

        void edit_render(Renderer& dc, bool selected) override;
        bool imgui_update() override;

        void save(msg::Var& ar) override; // Save prefab
        void load(msg::Var& ar) override; // Load prefab

        void serialize(msg::Writer& ar) override;  // Save to scene
        void deserialize(msg::Value& ar) override; // Load to scene

        Sound& set_sound(std::string_view path);
        Sound& set_sound(SoundSource::Ptr sound);
        Sound& sound();

        std::string_view object_type() const override;

        Region<float> bounding_box() const override;

    protected:
        SoundSource::Ptr _sound;
        Sound            _alias{};
        float            _radius{};
    };


    class SceneRegion : public ObjectBase
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

        bool imgui_update();
        void edit_render(Renderer& dc);

    protected:
        msg::Var              _region;
        mutable bool          _need_update{};
        mutable Region<float> _bounding_box;
    };


    class SceneFactory
    {
        using fact_t = BasicSceneObject* (*)();

    public:
        SceneFactory();
        ~SceneFactory() = default;
        static SceneFactory& instance();

        void set_root(const std::string& startPath);

        BasicSceneObject* create(std::string_view type) const;
        BasicSceneObject* create(uint64_t uid) const;

        template <class T>
        SceneFactory& factory(std::string_view type, std::string_view label);

        bool load_prototypes(std::string_view type);
        bool save_prototypes(std::string_view type);

        template <class T>
        SceneFactory& load_factory(std::string_view type, std::string_view label);

        void imgui_workspace();
        void imgui_workspace_menu();
        void imgui_properties();
        void imgui_explorer();
        void imgui_file_explorer();
        void center_view();

    private:
        struct ClassInfo
        {
            void                              select(int32_t n);
            bool                              is_selected() const;
            fact_t                            fn;
            std::string_view                  id;
            std::string                       name;
            msg::Var                          items;
            int32_t                           selected{-1};
            int32_t                           active{-1};
            std::unique_ptr<BasicSceneObject> obj{};
        };
        void        show_properties(ClassInfo& info);
        void        reset_atlas_cache();
        Atlas::Pack load_atlas(msg::Var& el);

        std::unordered_map<std::string, ClassInfo, std::string_hash, std::equal_to<>>              _factory;
        std::unordered_map<uint64_t, msg::Var>                                                     _prefab;
        std::unordered_map<std::string, std::shared_ptr<Atlas>, std::string_hash, std::equal_to<>> _cache;
        std::vector<ClassInfo*>                                                                    _classes;
        std::string                                                                                _base_folder;
        std::string                                                                                _buff;
        ClassInfo*                                                                                 _edit{};
        ClassInfo*                                                                                 _explore{};
        bool _scroll_to_center = true;
        bool _edit_origin      = false;
        bool _edit_collision   = false;
    };


    template <class T>
    inline SceneFactory& SceneFactory::factory(std::string_view type, std::string_view label)
    {
        auto [it, inserted] = _factory.try_emplace(std::string(type), ClassInfo());
        if (inserted)
        {
            ClassInfo& nfo = it->second;
            nfo.id         = it->first;
            nfo.fn         = []() -> BasicSceneObject* { return new T(); };
            nfo.name       = label;
            nfo.obj.reset(nfo.fn());
            _classes.emplace_back(&nfo);
        }
        return *this;
    }

    template <class T>
    inline SceneFactory& SceneFactory::load_factory(std::string_view type, std::string_view label)
    {
        factory<T>(type, label);
        load_prototypes(type);
        return *this;
    }

    inline std::string_view SpriteSceneObject::object_type() const
    {
        return type_id;
    }


} // namespace fin
