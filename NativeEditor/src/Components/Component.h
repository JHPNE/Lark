#pragma once
#include <memory>
#include <string>
#include <cassert>

class GameEntity;

// Component types enum
enum class ComponentType {
	Transform = 0,
	Script,
	Count
};

class Component {
public:
	virtual ~Component() = default;

	GameEntity* GetOwner() const { return m_owner; }
	ComponentType GetType() const { return m_type;  }

protected:
	// Only derived components can be constructed
	Component(GameEntity* owner, ComponentType type)
		: m_owner(owner)
		, m_type(type) {
		assert(owner && "Component must have an owner!");
	}

private:
	GameEntity* m_owner;
	ComponentType m_type;
};