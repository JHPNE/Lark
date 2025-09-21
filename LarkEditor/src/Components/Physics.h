#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Utils/System/Serialization.h"
#include <string>

using namespace MathUtils;

class Physics : public Component, public ISerializable
{
  public:
    explicit Physics(GameEntity *owner) : Component(owner) {}

    [[nodiscard]] ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Physic; }

    bool Initialize(const ComponentInitializer *init) override
    {
        if (init)
        {
            const auto *PhysicInit = static_cast<const PhysicInitializer *>(init);
            m_mass = PhysicInit->mass;
            m_is_kinematic = PhysicInit->is_kinematic;
            m_inertia = PhysicInit->inertia;

        }

        return true;
    };

    void SetMass(float mass) { m_mass = mass; }
    [[nodiscard]] float GetMass() const { return m_mass; }

    void SetKinematic(bool kinematic) { m_is_kinematic = kinematic; }
    [[nodiscard]] bool IsKinematic() const { return m_is_kinematic; }

    void SetInertia(const glm::vec3& inertia) { m_inertia = inertia; }
    [[nodiscard]] const glm::vec3& GetInertia() const { return m_inertia; }

    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
    {
        WriteVersion(element);

        SERIALIZE_PROPERTY(element, context, m_mass);
        SERIALIZE_PROPERTY(element, context, m_is_kinematic);
        SERIALIZE_VEC3(context, element, "Inertia", m_inertia);

    }

    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {

        context.version = ReadVersion(element);
        if (!SupportsVersion(context.version))
        {
            context.AddError("Unsupported Transform version: " + context.version.toString());
            return false;
        }

        DESERIALIZE_PROPERTY(element, context, m_mass);
        DESERIALIZE_PROPERTY(element, context, m_is_kinematic);
        DESERIALIZE_VEC3(element, "Inertia", m_inertia, glm::vec3(1.0f, 1.0f, 1.0f));

        return !context.HasErrors();
    }

    [[nodiscard]] Version GetVersion() const override { return {1, 1, 0}; };

  private:
    float m_mass;
    bool m_is_kinematic;
    glm::vec3 m_inertia;
};