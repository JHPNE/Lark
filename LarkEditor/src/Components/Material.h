#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Utils/System/Serialization.h"
#include <string>

using namespace MathUtils;

class Material : public Component, public ISerializable
{
    public:
        explicit Material(GameEntity *owner) : Component(owner) {}

        [[nodiscard]] ComponentType GetType() const override { return GetStaticType(); }
        static ComponentType GetStaticType() { return ComponentType::Material; }


        bool Initialize(const ComponentInitializer *init) override
        {
            if (init)
            {
                const auto *MaterialInit = static_cast<const MaterialInitializer *>(init);
                m_material = MaterialInit->material;
            }

            return true;
        };

        void SetMaterialType(MaterialType type) {m_material.type = type;}
        [[nodiscard]] MaterialType GetMaterialType() const {return m_material.type;}

        void SetAlbedo(glm::vec3 albedo) {m_material.albedo = albedo;}
        [[nodiscard]] glm::vec3 GetAlbedo() const { return m_material.albedo;}

        void SetRoughness(float roughness) {m_material.roughness = roughness;}
        [[nodiscard]] float GetRoughness() const { return m_material.roughness;}

        void SetMetallic(float metallic) {m_material.metallic = metallic;}
        [[nodiscard]] float GetMetallic() const { return m_material.metallic;}

        void SetNormal(glm::vec3 normal) {m_material.normal = normal;}
        [[nodiscard]] glm::vec3 GetNormal() const { return m_material.normal;}

        void SetEmissive(glm::vec3 emissive) {m_material.emissive = emissive;}
        [[nodiscard]] glm::vec3 GetEmissive() const { return m_material.emissive;}

        void SetIOR(float ior) {m_material.ior = ior;}
        [[nodiscard]] float GetIOR() const { return m_material.ior;}

        void SetTransparency(float transparency) {m_material.transparency = transparency;}
        [[nodiscard]] float GetTransparency() const { return m_material.transparency;}

        void SetAO(float ao) {m_material.ao = ao;}
        [[nodiscard]] float GetAO() const { return m_material.ao;}

        [[nodiscard]] const PBRMaterial& GetMaterialData() const { return m_material; }

        void Serialize(tinyxml2::XMLElement * element, SerializationContext &context) const override
        {
            WriteVersion(element);

            uint32_t materialType = static_cast<uint32_t>(m_material.type);
            SERIALIZE_PROPERTY(element, context, materialType);
            SERIALIZE_VEC3(context, element, "Albedo", m_material.albedo);
            SERIALIZE_PROPERTY(element, context, m_material.roughness);
            SERIALIZE_VEC3(context, element, "Normal", m_material.normal);
            SERIALIZE_VEC3(context, element, "Emissive", m_material.emissive);
            SERIALIZE_PROPERTY(element, context, m_material.ior);
            SERIALIZE_PROPERTY(element, context, m_material.transparency);
            SERIALIZE_PROPERTY(element, context, m_material.ao);
            SERIALIZE_PROPERTY(element, context, m_material.metallic);
        }

        bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
        {
            context.version = ReadVersion(element);
            if (!SupportsVersion(context.version))
            {
                context.AddError("Unsupported Material version: " + context.version.toString());
                return false;
            }

            uint32_t materialType;
            DESERIALIZE_PROPERTY(element, context, materialType);
            m_material.type = static_cast<MaterialType>(materialType);

            DESERIALIZE_VEC3(element, "Albedo", m_material.albedo, glm::vec3(1.0f));
            DESERIALIZE_PROPERTY(element, context, m_material.roughness);
            DESERIALIZE_VEC3(element, "Normal", m_material.albedo, glm::vec3(1.0f));
            DESERIALIZE_VEC3(element, "Emissive", m_material.albedo, glm::vec3(1.0f));
            DESERIALIZE_PROPERTY(element, context, m_material.ior);
            DESERIALIZE_PROPERTY(element, context, m_material.transparency);
            DESERIALIZE_PROPERTY(element, context, m_material.ao);
            DESERIALIZE_PROPERTY(element, context, m_material.metallic);

            return !context.HasErrors();
        }

        [[nodiscard]] Version GetVersion() const override { return {1, 1, 0}; };

    private:
        PBRMaterial m_material;
};