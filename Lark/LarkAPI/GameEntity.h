#pragma once

#include "../Common/CommonHeaders.h"
#include "../Components/ComponentCommon.h"
#include "GeometryComponent.h"
#include "PhysicsComponent.h"
#include "DroneComponent.h"
#include "ScriptComponent.h"
#include "TransformComponent.h"
#include "MaterialComponent.h"
#include <pybind11/embed.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace lark
{

namespace game_entity
{

DEFINE_TYPED_ID(entity_id);

class entity
{
  public:
    constexpr explicit entity(entity_id id) : _id{id} {}
    constexpr entity() : _id{id::invalid_id} {}
    constexpr entity_id get_id() const { return _id; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

    transform::component transform() const;
    script::component script() const;
    geometry::component geometry() const;
    physics::component physics() const;
    drone::component drone() const;
    material::component material() const;

  private:
    entity_id _id;
};
} // namespace game_entity

namespace geometry
{
class entity_geometry : public game_entity::entity
{
  public:
    virtual ~entity_geometry() = default;

  private:
};

namespace detail
{
using geometry_ptr = std::unique_ptr<geometry::entity_geometry>;
using geometry_creator = geometry_ptr (*)(game_entity::entity entity);

template <class geometry_class> geometry_ptr create_geometry(game_entity::entity entity)
{
    assert(entity.is_valid());
    return std::make_unique<geometry_class>(entity);
}
} // namespace detail
} // namespace geometry

namespace physics
{
class entity_physics : public game_entity::entity
{
  public:
    virtual ~entity_physics() = default;

  private:
};
} // namespace physics

namespace drone
{
class entity_drone : public game_entity::entity
{
    public:
        virtual ~entity_drone() = default;
};
}
namespace material
{
class entity_material : public game_entity::entity
{
    public:
    virtual ~entity_material() = default;
};
}

namespace script
{
class entity_script : public game_entity::entity
{

  public:
    virtual ~entity_script() = default;

    void begin_play()
    {
        try
        {
            if (m_instance && pybind11::hasattr(m_instance, "begin_play"))
            {
                m_instance.attr("begin_play")();
            }
        }
        catch (const pybind11::error_already_set &e)
        {
        }
    }

    void update(float dt)
    {
        try
        {
            if (m_instance && pybind11::hasattr(m_instance, "update"))
            {
                m_instance.attr("update")(dt);
            }
        }
        catch (const pybind11::error_already_set &e)
        {
        }
    }

  protected:
    explicit entity_script(game_entity::entity entity) : game_entity::entity{entity.get_id()}
    {
        try
        {
            static pybind11::scoped_interpreter guard{};
            // Assuming scripts are in a "scripts" folder relative to executable
            const char *script_name = typeid(*this).name();
            m_module = pybind11::module_::import(script_name);

            if (pybind11::hasattr(m_module, "Script"))
            {
                m_instance = m_module.attr("Script")(entity);
            }
        }
        catch (const pybind11::error_already_set &e)
        {
        }
    }

  private:
    pybind11::module m_module;
    pybind11::object m_instance;
};

namespace detail
{
using script_ptr = std::unique_ptr<entity_script>;
using script_creator = script_ptr (*)(game_entity::entity entity);
using string_hash = std::hash<std::string>;

u8 register_script(size_t, script_creator);
script_creator get_script_creator(size_t tag);
void get_script_names(const char **names, size_t *count);

template <class script_class> script_ptr create_script(game_entity::entity entity)
{
    assert(entity.is_valid());
    return std::make_unique<script_class>(entity);
}

u8 add_script_name(const char *name);
bool script_exists(size_t tag);
#define REGISTER_SCRIPT(TYPE)                                                                      \
    namespace                                                                                      \
    {                                                                                              \
    const u8 _reg## TYPE{lark::script::detail::register_script(                                     \
        lark::script::detail::string_hash()(#TYPE), &lark::script::detail::create_script<TYPE>)};  \
    const u8 name## TYPE{lark::script::detail::add_script_name(#TYPE)};                             \
    }
} // namespace detail
} // namespace script
} // namespace lark
