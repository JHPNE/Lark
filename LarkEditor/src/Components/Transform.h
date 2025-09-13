#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Utils/System/Serialization.h"
#include <string>

using namespace MathUtils;

class Transform : public Component, public ISerializable
{
  public:
    explicit Transform(GameEntity *owner)
        : Component(owner), m_position(0.0f, 0.0f, 0.0f), m_rotation(0.0f, 0.0f, 0.0f),
          m_scale(1.0f, 1.0f, 1.0f)
    {
    }

    // Component interface implementation
    [[nodiscard]] ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Transform; }

    // Position
    [[nodiscard]] const glm::vec3 &GetPosition() const { return m_position; }
    void SetPosition(const glm::vec3 &position) { m_position = position; }
    void SetPosition(float x, float y, float z) { m_position = glm::vec3(x, y, z); }

    // Rotation
    [[nodiscard]] const glm::vec3 &GetRotation() const { return m_rotation; }
    void SetRotation(const glm::vec3 &rotation) { m_rotation = rotation; }
    void SetRotation(float x, float y, float z) { m_rotation = glm::vec3(x, y, z); }

    // Scale
    [[nodiscard]] const glm::vec3 &GetScale() const { return m_scale; }
    void SetScale(const glm::vec3 &scale) { m_scale = scale; }
    void SetScale(float x, float y, float z) { m_scale = glm::vec3(x, y, z); }
    void SetScale(float uniform) { m_scale = glm::vec3(uniform, uniform, uniform); }

    // Utility functions
    void Reset()
    {
        m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    void packForEngine(transform_component *transform_component) const
    {
        transform_component->position[0] = this->GetPosition().x;
        transform_component->position[1] = this->GetPosition().y;
        transform_component->position[2] = this->GetPosition().z;

        transform_component->rotation[0] = this->GetRotation().x;
        transform_component->rotation[1] = this->GetRotation().y;
        transform_component->rotation[2] = this->GetRotation().z;

        transform_component->scale[0] = this->GetScale().x;
        transform_component->scale[1] = this->GetScale().y;
        transform_component->scale[2] = this->GetScale().z;
    }

    float *loadFromEngine(const transform_component *transform_component)
    {
        static float value[9];
        value[0] = transform_component->position[0];
        value[1] = transform_component->position[1];
        value[2] = transform_component->position[2];

        value[3] = transform_component->rotation[0];
        value[4] = transform_component->rotation[1];
        value[5] = transform_component->rotation[2];

        value[6] = transform_component->scale[0];
        value[7] = transform_component->scale[1];
        value[8] = transform_component->scale[2];

        // Update within Component aswell
        SetPosition(value[0], value[1], value[2]);
        SetRotation(value[3], value[4], value[5]);
        SetScale(value[6], value[7], value[8]);

        return value;
    }

    // Serialization interface
    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
    {
        WriteVersion(element);

        SERIALIZE_VEC3(context, element, "Position", m_position);
        SERIALIZE_VEC3(context, element, "Rotation", m_rotation);
        SERIALIZE_VEC3(context, element, "Scale", m_scale);
    }

    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {

        context.version = ReadVersion(element);
        if (!SupportsVersion(context.version))
        {
            context.AddError("Unsupported Transform version: " + context.version.toString());
            return false;
        }

        DESERIALIZE_VEC3(element, "Position", m_position, glm::vec3(0, 0, 0));
        DESERIALIZE_VEC3(element, "Rotation", m_rotation, glm::vec3(0, 0, 0));
        DESERIALIZE_VEC3(element, "Scale", m_scale, glm::vec3(1, 1, 1));

        return !context.HasErrors();
    }

    [[nodiscard]] Version GetVersion() const override { return {1, 1, 0}; };

  private:
    glm::vec3 m_position; // Local position
    glm::vec3 m_rotation; // Local rotation in degrees
    glm::vec3 m_scale;    // Local scale
};