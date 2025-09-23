// LarkEditor/src/ViewModels/ProjectSettingsViewModel.h
#pragma once
#include "../Utils/System/Serialization.h"
#include <memory>
#include "EngineAPI.h"
#include <glm/glm.hpp>

struct CameraSettings : ISerializable
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    float distance{10.0f};
    float fov{45.0f};
    float nearPlane{0.1f};
    float farPlane{1000.0f};
    float moveSpeed{5.0f};
    float rotateSpeed{1.0f};
    float zoomSpeed{0.5f};

    void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override
    {
        SERIALIZE_VEC3(context, element, "Position", position);
        SERIALIZE_VEC3(context, element, "Rotation", rotation);
        SERIALIZE_PROPERTY(element, context, distance);
        SERIALIZE_PROPERTY(element, context, fov);
        SERIALIZE_PROPERTY(element, context, nearPlane);
        SERIALIZE_PROPERTY(element, context, farPlane);
        SERIALIZE_PROPERTY(element, context, moveSpeed);
        SERIALIZE_PROPERTY(element, context, rotateSpeed);
        SERIALIZE_PROPERTY(element, context, zoomSpeed);
    }

    bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override
    {
        DESERIALIZE_VEC3(element, "Position", position, glm::vec3(0.0f));
        DESERIALIZE_VEC3(element, "Rotation", rotation, glm::vec3(0.0f));
        DESERIALIZE_PROPERTY(element, context, distance);
        DESERIALIZE_PROPERTY(element, context, fov);
        DESERIALIZE_PROPERTY(element, context, nearPlane);
        DESERIALIZE_PROPERTY(element, context, farPlane);
        DESERIALIZE_PROPERTY(element, context, moveSpeed);
        DESERIALIZE_PROPERTY(element, context, rotateSpeed);
        DESERIALIZE_PROPERTY(element, context, zoomSpeed);
        return true;
    }

    Version GetVersion() const override { return {1, 0, 0}; }

    bool operator==(const CameraSettings& other) const {
        return position == other.position &&
               rotation == other.rotation &&
               distance == other.distance &&
               fov == other.fov &&
               nearPlane == other.nearPlane &&
               farPlane == other.farPlane &&
               moveSpeed == other.moveSpeed &&
               rotateSpeed == other.rotateSpeed &&
               zoomSpeed == other.zoomSpeed;
    }

    bool operator!=(const CameraSettings& other) const {
        return !(*this == other);
    }
};

struct WorldSettings : ISerializable
{
glm::vec3 gravity{0.0f, -9.81f, 0.0f};
wind_type windType{wind_type::NoWind};
glm::vec3 windVector{0.0f, 0.0f, 0.0f};
glm::vec3 windAmplitudes{1.0f, 1.0f, 1.0f};
glm::vec3 windFrequencies{1.0f, 1.0f, 1.0f};
float timeScale{1.0f};
int physicsIterations{10};
float fixedTimeStep{0.01667f}; // 60 FPS

void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override
{
    SERIALIZE_VEC3(context, element, "Gravity", gravity);
    SerializerUtils::WriteAttribute(element, "WindType", static_cast<int>(windType));
    SERIALIZE_VEC3(context, element, "WindVector", windVector);
    SERIALIZE_VEC3(context, element, "WindAmplitudes", windAmplitudes);
    SERIALIZE_VEC3(context, element, "WindFrequencies", windFrequencies);
    SERIALIZE_PROPERTY(element, context, timeScale);
    SERIALIZE_PROPERTY(element, context, physicsIterations);
    SERIALIZE_PROPERTY(element, context, fixedTimeStep);
}

bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override
    {
        DESERIALIZE_VEC3(element, "Gravity", gravity, glm::vec3(0.0f, -9.81f, 0.0f));
        int windTypeInt = 0;
        SerializerUtils::ReadAttribute(element, "WindType", windTypeInt);
        windType = static_cast<wind_type>(windTypeInt);
        DESERIALIZE_VEC3(element, "WindVector", windVector, glm::vec3(0.0f));
        DESERIALIZE_VEC3(element, "WindAmplitudes", windAmplitudes, glm::vec3(1.0f));
        DESERIALIZE_VEC3(element, "WindFrequencies", windFrequencies, glm::vec3(1.0f));
        DESERIALIZE_PROPERTY(element, context, timeScale);
        DESERIALIZE_PROPERTY(element, context, physicsIterations);
        DESERIALIZE_PROPERTY(element, context, fixedTimeStep);
        return true;
    }

    Version GetVersion() const override { return {1, 0, 0}; }

    bool operator==(const WorldSettings& other) const {
        return gravity == other.gravity &&
               windType == other.windType &&
               windVector == other.windVector &&
               windAmplitudes == other.windAmplitudes &&
               windFrequencies == other.windFrequencies &&
               timeScale == other.timeScale &&
               physicsIterations == other.physicsIterations &&
               fixedTimeStep == other.fixedTimeStep;
    }

    bool operator!=(const WorldSettings& other) const {
        return !(*this == other);
    }
};

struct RenderSettings : ISerializable
{
    bool enableWireframe{false};
    bool enableLighting{true};
    bool enableShadows{false};
    bool enableVSync{true};
    glm::vec3 ambientColor{0.1f, 0.1f, 0.1f};
    glm::vec3 sunDirection{-0.5f, -1.0f, -0.5f};
    glm::vec3 sunColor{1.0f, 0.95f, 0.8f};
    float sunIntensity{1.0f};

    void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override
    {
        SERIALIZE_PROPERTY(element, context, enableWireframe);
        SERIALIZE_PROPERTY(element, context, enableLighting);
        SERIALIZE_PROPERTY(element, context, enableShadows);
        SERIALIZE_PROPERTY(element, context, enableVSync);
        SERIALIZE_VEC3(context, element, "AmbientColor", ambientColor);
        SERIALIZE_VEC3(context, element, "SunDirection", sunDirection);
        SERIALIZE_VEC3(context, element, "SunColor", sunColor);
        SERIALIZE_PROPERTY(element, context, sunIntensity);
    }

    bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override
    {
        DESERIALIZE_PROPERTY(element, context, enableWireframe);
        DESERIALIZE_PROPERTY(element, context, enableLighting);
        DESERIALIZE_PROPERTY(element, context, enableShadows);
        DESERIALIZE_PROPERTY(element, context, enableVSync);
        DESERIALIZE_VEC3(element, "AmbientColor", ambientColor, glm::vec3(0.1f));
        DESERIALIZE_VEC3(element, "SunDirection", sunDirection, glm::vec3(-0.5f, -1.0f, -0.5f));
        DESERIALIZE_VEC3(element, "SunColor", sunColor, glm::vec3(1.0f, 0.95f, 0.8f));
        DESERIALIZE_PROPERTY(element, context, sunIntensity);
        return true;
    }

    Version GetVersion() const override { return {1, 0, 0}; }

    bool operator==(const RenderSettings& other) const {
        return enableWireframe == other.enableWireframe &&
               enableLighting == other.enableLighting &&
               enableShadows == other.enableShadows &&
               enableVSync == other.enableVSync &&
               ambientColor == other.ambientColor &&
               sunDirection == other.sunDirection &&
               sunColor == other.sunColor &&
               sunIntensity == other.sunIntensity;
    }

    bool operator!=(const RenderSettings& other) const {
        return !(*this == other);
    }
};
