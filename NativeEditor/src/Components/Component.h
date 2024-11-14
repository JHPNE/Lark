#pragma once
#include <cassert>
#include <memory>

class GameEntity;

enum class ComponentType {
	None = 0,
	Transform,
	Script,
	// Add other component types here
	Count
};

class Component {
public:
	virtual ~Component() = default;
	virtual ComponentType GetType() const = 0;

	// Static type getter that derived classes will override
	static ComponentType GetStaticType() { return ComponentType::None; }

	// Getter for owner
	GameEntity* GetOwner() const { return m_owner; }

protected:
	explicit Component(GameEntity* owner) : m_owner(owner) {
		// Assert owner is not null in debug builds
		assert(owner != nullptr);
	}

	// Prevent copying
	Component(const Component&) = delete;
	Component& operator=(const Component&) = delete;

private:
	GameEntity* m_owner;
};