#pragma once
#include <cassert>
#include <memory>
#include <string>

#include "EngineAPI.h"
#include "Utils/MathUtils.h"

class GameEntity;

class Component;
class Transform;
class Script;
class Geometry;
class Physics;
class Drone;
class Scene;

using namespace MathUtils;

// Base struct for component initialization data
struct ComponentInitializer
{
    virtual ~ComponentInitializer() = default;
};

// Transform component initializer
struct TransformInitializer : ComponentInitializer
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

// Script component initializer
struct ScriptInitializer : ComponentInitializer
{
    std::string scriptName;
    // Add any other script-specific initialization data
};

struct GeometryInitializer : ComponentInitializer
{
    std::string geometryName;
    GeometryType geometryType;
    bool visible;
    std::string geometrySource;
    content_tools::PrimitiveMeshType meshType;
};

struct PhysicInitializer : ComponentInitializer
{
    float mass{1.0f};
    glm::vec3 inertia{glm::vec3(0.f)};
    bool is_kinematic{false};
};

struct DroneInitializer : ComponentInitializer
{
    quad_params params;
    control_abstraction control_abstraction;
    trajectory trajectory;
    drone_state drone_state;
    control_input input;
};

enum class ComponentType
{
    None = 0,
    Transform,
    Script,
    Geometry,
    Physic,
    Drone
    // Add other component types here
};

class Component
{
  public:
    virtual ~Component() = default;
    virtual ComponentType GetType() const = 0;
    virtual bool Initialize(const ComponentInitializer *init) { return true; }

    static ComponentType GetStaticType() { return ComponentType::None; }
    GameEntity *GetOwner() const { return m_owner; }

    static const char *ComponentTypeToString(ComponentType type)
    {
        switch (type)
        {
        case ComponentType::None:
            return "None";
        case ComponentType::Transform:
            return "Transform";
        case ComponentType::Script:
            return "Script";
        case ComponentType::Geometry:
            return "Geometry";
        case ComponentType::Physic:
            return "Physic";
        case ComponentType::Drone:
            return "Drone";
        default:
            return "Unknown";
        }
    }

  protected:
    explicit Component(GameEntity *owner) : m_owner(owner) { assert(owner != nullptr); }

    Component(const Component &) = delete;
    Component &operator=(const Component &) = delete;

  private:
    GameEntity *m_owner;
};