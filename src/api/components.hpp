#pragma once

#include "types.hpp"

namespace fin
{
    extern GameAPI gGameAPI;

    enum ComponentsFlags_
    {
        ComponentsFlags_Default           = 0,      // Default component flags
        ComponentsFlags_Private           = 1 << 1, // Component is private and should not be shown in the component list
        ComponentsFlags_NoWorkspaceEditor = 1 << 2, // Component should not be shown in the workspace editor
        ComponentsFlags_NoEditor          = 1 << 3, // Component should not be shown in the editor at all
        ComponentsFlags_NoPrefab          = 1 << 4,
    };
    using ComponentsFlags = uint32_t;



    struct ComponentInfo
    {
        void Serialize(Entity e, fin::msg::Var& ar) const;
        void Deserialize(Entity e, fin::msg::Var& ar) const;
        void Edit(Entity e) const;
        void EditCanvas(Entity e, ImGui::CanvasParams& cp) const;

        using serialize_fn   = void (*)(void*, Entity, fin::msg::Var&);
        using entity_fn      = void (*)(void*, Entity);
        using entity_edit_fn = void (*)(void*, Entity, ImGui::CanvasParams&);

        std::string_view name;
        std::string_view id;
        std::string_view label;
        uint32_t         index       = 0;
        ComponentsFlags  flags       = 0;       // flags for the component
        SparseSet*       storage     = nullptr; // owned only if not external
        PluginHandle     owner       = nullptr; // owner plugin of the component
        serialize_fn     serialize   = nullptr;
        serialize_fn     deserialize = nullptr;
        entity_fn        edit        = nullptr;
        entity_edit_fn   edit_canvas = nullptr;
    };

    template <typename T>
    struct ComponentInfoStorage : ComponentInfo
    {
        entt::storage<T> set;
    };


    class ScriptComponent
    {
    public:
        ScriptComponent()                    = default;
        virtual ~ScriptComponent()           = default;
        virtual void Init(Entity entity)     = 0; // Initialize the component for the given entity
        virtual void Update(float deltaTime) = 0; // Update the component logic
    };



    struct DataComponent
    {
    };



    template <typename T>
    struct ComponentTraits
    {
        static ComponentId              component_id; // Unique ID for the component type
        static entt::storage<T>*        storage;
        static ComponentInfoStorage<T>* info;
    };

    template <typename T>
    ComponentId ComponentTraits<T>::component_id{0xffffffff};

    template <typename T>
    entt::storage<T>* ComponentTraits<T>::storage{};

    template <typename T>
    ComponentInfoStorage<T>* ComponentTraits<T>::info{};




    template <typename T>
    concept HasSerialize = requires(const T& t, Entity e, fin::msg::Var& v)
    {
        {
            t.Serialize(e, v)
        } -> std::same_as<void>;
    };

    template <typename T>
    concept HasDeserialize = requires(T & t, Entity e, fin::msg::Var& v)
    {
        {
            t.Deserialize(e, v)
        } -> std::same_as<void>;
    };

    template <typename T>
    concept HasEdit = requires(T & t, Entity e)
    {
        {
            t.ImguiProps(e)
        } -> std::same_as<void>;
    };

    template <typename T>
    concept HasEditCanvas = requires(T & t, Entity e, ImGui::CanvasParams& c)
    {
        {
            t.ImguiWorkspace(e, c)
        } -> std::same_as<void>;
    };

    inline void ComponentInfo::Serialize(Entity e, fin::msg::Var& ar) const
    {
        serialize(storage->get(e), e, ar);
    }

    inline void ComponentInfo::Deserialize(Entity e, fin::msg::Var& ar) const
    {
        deserialize(storage->get(e), e, ar);
    }

    inline void ComponentInfo::Edit(Entity e) const
    {
        edit(storage->get(e), e);
    }

    inline void ComponentInfo::EditCanvas(Entity e, ImGui::CanvasParams& cp) const
    {
        edit_canvas(storage->get(e), e, cp);
    }


    template <typename C>
    ComponentInfo* RegisterComponentInt(StringView name, StringView label, ComponentsFlags flags, IGamePlugin* owner)
    {
        auto* nfo    = new ComponentInfoStorage<C>();
        nfo->index   = 0;
        nfo->name    = entt::internal::stripped_type_name<C>();
        nfo->id      = name;
        nfo->label   = label.empty() ? nfo->name : label;
        nfo->flags   = flags;
        nfo->owner   = owner;
        nfo->storage = &nfo->set;

        if constexpr (HasSerialize<C>)
        {
            nfo->serialize = [](void* self, Entity e, msg::Var& ar) { static_cast<const C*>(self)->Serialize(e, ar); };
        }

        if constexpr (HasDeserialize<C>)
        {
            nfo->deserialize = [](void* self, Entity e, msg::Var& ar) { static_cast<C*>(self)->Deserialize(e, ar); };
        }

        if constexpr (HasEdit<C>)
        {
            nfo->edit = [](void* self, Entity e) { static_cast<C*>(self)->ImguiProps(e); };
        }

        if constexpr (HasEditCanvas<C>)
        {
            nfo->edit_canvas = [](void* self, Entity e, ImGui::CanvasParams& cp)
            { static_cast<C*>(self)->ImguiWorkspace(e, cp); };
        }

        gGameAPI.RegisterComponentInfo(gGameAPI, nfo);
        ComponentTraits<C>::info         = nfo;
        ComponentTraits<C>::storage      = &nfo->set;
        ComponentTraits<C>::component_id = nfo->index;

        return nfo;
    }


} // namespace fin
