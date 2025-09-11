// LarkEditor/src/Models/GeometryModel.h
#pragma once
#include "../Geometry/Geometry.h"
#include "EngineAPI.h"
#include <memory>
#include <unordered_map>
#include <optional>

// Represents a single geometry instance
struct GeometryInstance
{
    uint32_t entityId;
    std::string name;
    GeometryType type;
    bool visible;
    std::unique_ptr<lark::editor::Geometry> geometryData;
    content_tools::scene sceneData;

    // For primitives
    std::optional<content_tools::PrimitiveMeshType> primitiveType;
    std::optional<glm::vec3> size;
    std::optional<uint32_t> segments[3];
    std::optional<uint32_t> lod;

    // For imported meshes
    std::optional<std::string> sourcePath;

    // Runtime state
    bool needsUpdate = false;
    glm::mat4 transform = glm::mat4(1.0f);
};

// Manages all geometry data in the application
class GeometryModel
{
public:
    using GeometryMap = std::unordered_map<uint32_t, std::unique_ptr<GeometryInstance>>;

    GeometryModel() = default;
    ~GeometryModel() = default;

    // Geometry management
    bool AddGeometry(uint32_t entityId, std::unique_ptr<GeometryInstance> geometry)
    {
        if (m_geometries.find(entityId) != m_geometries.end())
        {
            return false; // Already exists
        }
        m_geometries[entityId] = std::move(geometry);
        return true;
    }

    bool RemoveGeometry(uint32_t entityId)
    {
        return m_geometries.erase(entityId) > 0;
    }

    GeometryInstance* GetGeometry(uint32_t entityId)
    {
        auto it = m_geometries.find(entityId);
        return it != m_geometries.end() ? it->second.get() : nullptr;
    }

    const GeometryInstance* GetGeometry(uint32_t entityId) const
    {
        auto it = m_geometries.find(entityId);
        return it != m_geometries.end() ? it->second.get() : nullptr;
    }

    const GeometryMap& GetAllGeometries() const { return m_geometries; }

    // Clear all geometries
    void Clear()
    {
        m_geometries.clear();
    }

    // Check if entity has geometry
    bool HasGeometry(uint32_t entityId) const
    {
        return m_geometries.find(entityId) != m_geometries.end();
    }

    // Update geometry data
    bool UpdateGeometryData(uint32_t entityId, content_tools::scene* newSceneData)
    {
        auto* geom = GetGeometry(entityId);
        if (!geom || !newSceneData)
            return false;

        geom->sceneData = *newSceneData;
        geom->needsUpdate = true;
        return true;
    }

    // Mark geometry for update
    void MarkForUpdate(uint32_t entityId)
    {
        if (auto* geom = GetGeometry(entityId))
        {
            geom->needsUpdate = true;
        }
    }

    std::vector<uint32_t> GetEntitiesNeedingUpdate() const
    {
        std::vector<uint32_t> result;
        for (const auto& [id, geom] : m_geometries)
        {
            if (geom->needsUpdate)
            {
                result.push_back(id);
            }
        }
        return result;
    }

private:
    GeometryMap m_geometries;
};