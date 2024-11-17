#pragma once
#include <Utils/System/Serialization.h>

#include "Component.h"
#include "tinyxml2.h"
#include "../Utils/MathUtils.h" // Assuming you have a math utility header

using namespace MathUtils;

class Transform : public Component, public ISerializable {
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

    // Serialization interface
    void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override {
        // Create components for each vector
        auto positionElement = context.document.NewElement("Position");
        SerializerUtils::WriteAttribute(positionElement, "x", m_position.x);
        SerializerUtils::WriteAttribute(positionElement, "y", m_position.y);
        SerializerUtils::WriteAttribute(positionElement, "z", m_position.z);
        element->LinkEndChild(positionElement);

        auto rotationElement = context.document.NewElement("Rotation");
        SerializerUtils::WriteAttribute(rotationElement, "x", m_rotation.x);
        SerializerUtils::WriteAttribute(rotationElement, "y", m_rotation.y);
        SerializerUtils::WriteAttribute(rotationElement, "z", m_rotation.z);
        element->LinkEndChild(rotationElement);

        auto scaleElement = context.document.NewElement("Scale");
        SerializerUtils::WriteAttribute(scaleElement, "x", m_scale.x);
        SerializerUtils::WriteAttribute(scaleElement, "y", m_scale.y);
        SerializerUtils::WriteAttribute(scaleElement, "z", m_scale.z);
        element->LinkEndChild(scaleElement);
    }

    bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override {
        // Read position
        if (auto positionElement = element->FirstChildElement("Position")) {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            SerializerUtils::ReadAttribute(positionElement, "x", x);
            SerializerUtils::ReadAttribute(positionElement, "y", y);
            SerializerUtils::ReadAttribute(positionElement, "z", z);
            m_position = Vec3(x, y, z);
        }

        // Read rotation
        if (auto rotationElement = element->FirstChildElement("Rotation")) {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            SerializerUtils::ReadAttribute(rotationElement, "x", x);
            SerializerUtils::ReadAttribute(rotationElement, "y", y);
            SerializerUtils::ReadAttribute(rotationElement, "z", z);
            m_rotation = Vec3(x, y, z);
        }

        // Read scale
        if (auto scaleElement = element->FirstChildElement("Scale")) {
            float x = 1.0f, y = 1.0f, z = 1.0f;  // Default scale is 1
            SerializerUtils::ReadAttribute(scaleElement, "x", x);
            SerializerUtils::ReadAttribute(scaleElement, "y", y);
            SerializerUtils::ReadAttribute(scaleElement, "z", z);
            m_scale = Vec3(x, y, z);
        }

        return true;
    }

private:
    Vec3 m_position;  // Local position
    Vec3 m_rotation;  // Local rotation in degrees
    Vec3 m_scale;     // Local scale
};