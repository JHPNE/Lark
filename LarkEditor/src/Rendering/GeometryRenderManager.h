#pragma once
#include "../Rendering/GeometryRenderer.h"
#include "../Models/GeometryModel.h"
#include "../Utils/Etc/Logger.h"
#include <unordered_map>
#include <memory>

class GeometryRenderManager
{
public:
    struct RenderableGeometry
    {
        uint32_t entityId;
        std::unique_ptr<GeometryRenderer::LODGroupBuffers> buffers;
        bool visible = true;
        bool needsBufferUpdate = false;
    };

    GeometryRenderManager() = default;
    ~GeometryRenderManager()
    {
        ClearAll();
    }

    // Create or update render buffers for geometry
    bool CreateOrUpdateBuffers(uint32_t entityId, content_tools::scene* sceneData)
    {
        if (!sceneData)
        {
            Logger::Get().Log(MessageType::Error,
                "Null scene data for entity " + std::to_string(entityId));
            return false;
        }

        auto buffers = GeometryRenderer::CreateBuffersFromGeometry(sceneData);
        if (!buffers)
        {
            Logger::Get().Log(MessageType::Error,
                "Failed to create buffers for entity " + std::to_string(entityId));
            return false;
        }

        // Create or update the renderable
        auto& renderable = m_renderables[entityId];
        if (!renderable)
        {
            renderable = std::make_unique<RenderableGeometry>();
            renderable->entityId = entityId;
            renderable->visible = true;
        }

        renderable->buffers = std::move(buffers);
        renderable->needsBufferUpdate = false;

        Logger::Get().Log(MessageType::Info,
            "Created/Updated render buffers for entity " + std::to_string(entityId));

        return true;
    }

    // Remove render buffers
    bool RemoveBuffers(uint32_t entityId)
    {
        return m_renderables.erase(entityId) > 0;
    }

    // Render geometry
    void RenderGeometry(uint32_t entityId, const glm::mat4& view,
                       const glm::mat4& projection, float distanceToCamera)
    {
        auto it = m_renderables.find(entityId);
        if (it == m_renderables.end() || !it->second || !it->second->visible)
            return;

        GeometryRenderer::RenderGeometryAtLOD(
            it->second->buffers.get(), view, projection, distanceToCamera);
    }

    // Render all visible geometries
    void RenderAll(const glm::mat4& view, const glm::mat4& projection,
                   float distanceToCamera, const std::function<glm::mat4(uint32_t)>& getTransform)
    {
        for (const auto& [entityId, renderable] : m_renderables)
        {
            if (!renderable || !renderable->visible)
                continue;

            glm::mat4 model = getTransform ? getTransform(entityId) : glm::mat4(1.0f);
            glm::mat4 finalView = view * model;

            GeometryRenderer::RenderGeometryAtLOD(
                renderable->buffers.get(), finalView, projection, distanceToCamera);
        }
    }

    // Set visibility
    void SetVisible(uint32_t entityId, bool visible)
    {
        if (auto it = m_renderables.find(entityId); it != m_renderables.end())
        {
            it->second->visible = visible;
        }
    }

    // Check if entity has render buffers
    bool HasBuffers(uint32_t entityId) const
    {
        return m_renderables.find(entityId) != m_renderables.end();
    }

    // Clear all render buffers
    void ClearAll()
    {
        m_renderables.clear();
    }

    // Get renderable geometry
    RenderableGeometry* GetRenderable(uint32_t entityId)
    {
        auto it = m_renderables.find(entityId);
        return it != m_renderables.end() ? it->second.get() : nullptr;
    }

private:
    std::unordered_map<uint32_t, std::unique_ptr<RenderableGeometry>> m_renderables;
};