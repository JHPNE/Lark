#pragma once
#include "Component.h"
#include "../Utils/MathUtils.h" // Assuming you have a math utility header

using namespace MathUtils;

class Transform : public Component {
public:
    explicit Transform(GameEntity* owner) : Component(owner),
        m_position(0.0f, 0.0f, 0.0f),
        m_rotation(0.0f, 0.0f, 0.0f),
        m_scale(1.0f, 1.0f, 1.0f)
    {}

    // Component interface implementation
    ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Transform; }

    // Position
    const Vec3& GetPosition() const { return m_position; }
    void SetPosition(const Vec3& position) { m_position = position; }
    void SetPosition(float x, float y, float z) { m_position = Vec3(x, y, z); }

    // Rotation
    const Vec3& GetRotation() const { return m_rotation; }
    void SetRotation(const Vec3& rotation) { m_rotation = rotation; }
    void SetRotation(float x, float y, float z) { m_rotation = Vec3(x, y, z); }

    // Scale
    const Vec3& GetScale() const { return m_scale; }
    void SetScale(const Vec3& scale) { m_scale = scale; }
    void SetScale(float x, float y, float z) { m_scale = Vec3(x, y, z); }
    void SetScale(float uniform) { m_scale = Vec3(uniform, uniform, uniform); }

    // Utility functions
    void Reset() {
        m_position = Vec3(0.0f, 0.0f, 0.0f);
        m_rotation = Vec3(0.0f, 0.0f, 0.0f);
        m_scale = Vec3(1.0f, 1.0f, 1.0f);
    }

private:
    Vec3 m_position;  // Local position
    Vec3 m_rotation;  // Local rotation in degrees
    Vec3 m_scale;     // Local scale
};