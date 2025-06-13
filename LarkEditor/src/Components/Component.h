#pragma once
#include <cassert>
#include <string>
#include <memory>
#include "Utils/MathUtils.h"

class GameEntity;

class Component;
class Transform;
class Script;
class Geometry;
class Scene;

using namespace MathUtils;

// Base struct for component initialization data
struct ComponentInitializer {
	virtual ~ComponentInitializer() = default;
};

// Transform component initializer
struct TransformInitializer : ComponentInitializer {
	Vec3 position{0.0f, 0.0f, 0.0f};
	Vec3 rotation{0.0f, 0.0f, 0.0f};
	Vec3 scale{1.0f, 1.0f, 1.0f};
};

// Script component initializer
struct ScriptInitializer : ComponentInitializer {
	std::string scriptName;
	// Add any other script-specific initialization data
};

struct GeometryInitializer : ComponentInitializer {
	char* geometryName;
};


enum class ComponentType {
	None = 0,
	Transform,
	Script,
	Geometry,
	// Add other component types here
};

class Component {
public:
	virtual ~Component() = default;
	virtual ComponentType GetType() const = 0;
	virtual bool Initialize(const ComponentInitializer* init) { return true; }

	static ComponentType GetStaticType() { return ComponentType::None; }
	GameEntity* GetOwner() const { return m_owner; }

protected:
	explicit Component(GameEntity* owner) : m_owner(owner) {
		assert(owner != nullptr);
	}

	Component(const Component&) = delete;
	Component& operator=(const Component&) = delete;

private:
	GameEntity* m_owner;
};