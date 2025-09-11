// LarkEditor/src/Services/GeometryService.h
#pragma once
#include "../Models/GeometryModel.h"
#include "../Geometry/Geometry.h"
#include "EngineAPI.h"
#include "Utils/Etc/Logger.h"
#include <memory>

class GeometryService
{
public:
    static GeometryService& Get()
    {
        static GeometryService instance;
        return instance;
    }

    // Create primitive geometry
    std::unique_ptr<GeometryInstance> CreatePrimitive(
        content_tools::PrimitiveMeshType type,
        const glm::vec3& size,
        const uint32_t segments[3],
        uint32_t lod)
    {
        auto instance = std::make_unique<GeometryInstance>();
        instance->type = GeometryType::PrimitiveType;
        instance->primitiveType = type;
        instance->size = size;
        instance->lod = lod;

        // Store segments
        if (segments)
        {
            instance->segments[0] = segments[0];
            instance->segments[1] = segments[1];
            instance->segments[2] = segments[2];
        }

        // Create the actual geometry
        float sizeArray[3] = {size.x, size.y, size.z};
        instance->geometryData = lark::editor::Geometry::CreatePrimitive(
            type, sizeArray, segments, lod);

        if (!instance->geometryData)
        {
            Logger::Get().Log(MessageType::Error, "Failed to create primitive geometry");
            return nullptr;
        }

        // Get the scene data
        if (auto* scene = instance->geometryData->GetScene())
        {
            instance->sceneData = *scene;
        }

        return instance;
    }

    // Load geometry from file
    std::unique_ptr<GeometryInstance> LoadFromFile(const std::string& filepath)
    {
        auto instance = std::make_unique<GeometryInstance>();
        instance->type = GeometryType::ObjImport;
        instance->sourcePath = filepath;

        // Load the geometry
        instance->geometryData = lark::editor::Geometry::LoadGeometry(filepath.c_str());

        if (!instance->geometryData)
        {
            Logger::Get().Log(MessageType::Error,
                "Failed to load geometry from: " + filepath);
            return nullptr;
        }

        // Get the scene data
        if (auto* scene = instance->geometryData->GetScene())
        {
            instance->sceneData = *scene;
        }

        Logger::Get().Log(MessageType::Info,
            "Successfully loaded geometry from: " + filepath);

        return instance;
    }

    // Update entity geometry in engine
    bool UpdateEntityGeometry(uint32_t entityId, content_tools::scene* sceneData)
    {
        if (!sceneData)
            return false;

        game_entity_descriptor desc{};
        desc.geometry.scene = sceneData;
        desc.geometry.is_dynamic = false;

        return UpdateGameEntity(entityId, &desc);
    }

    // Get modified mesh data from engine
    bool GetModifiedMeshData(uint32_t entityId, content_tools::SceneData* outData)
    {
        if (!outData)
            return false;

        return ::GetModifiedMeshData(entityId, outData);
    }

    // Modify vertex positions
    bool ModifyVertexPositions(uint32_t entityId, const std::vector<glm::vec3>& vertices)
    {
        if (vertices.empty())
            return false;

        ModifyEntityVertexPositions(entityId, const_cast<std::vector<glm::vec3>&>(vertices));
        return true;
    }

    // Get entity transform from engine
    glm::mat4 GetEntityTransform(uint32_t entityId)
    {
        return GetEntityTransformMatrix(entityId);
    }

    // Set entity transform in engine
    bool SetEntityTransform(uint32_t entityId, const transform_component& transform)
    {
        return ::SetEntityTransform(entityId, transform);  // Call global function, not self
    }

    // Reset entity transform
    bool ResetEntityTransform(uint32_t entityId)
    {
        return ::ResetEntityTransform(entityId);  // Call global function, not self
    }

private:
    GeometryService() = default;
    ~GeometryService() = default;

    GeometryService(const GeometryService&) = delete;
    GeometryService& operator=(const GeometryService&) = delete;
};