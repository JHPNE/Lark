#pragma once
#include "Component.h"

struct Vector3 {
    float x{ 0 }, y{ 0 }, z{ 0 };
};

class Transform : public Component {
public:
    Transform(GameEntity* owner)
        : Component(owner, ComponentType::Transform) {}

    const Vector3& GetPosition() const { return m_position; }
    const Vector3& GetRotation() const { return m_rotation; }
    const Vector3& GetScale() const { return m_scale; }

    void SetPosition(const Vector3& position) { m_position = position; }
    void SetRotation(const Vector3& rotation) { m_rotation = rotation; }
    void SetScale(const Vector3& scale) { m_scale = scale; }

private:
    Vector3 m_position;
    Vector3 m_rotation;
    Vector3 m_scale{ 1, 1, 1 };
};