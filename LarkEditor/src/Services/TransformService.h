#pragma once
#include "../Utils/MathUtils.h"
#include "../Project/GameEntity.h"
#include "../Components/Transform.h"
#include "EngineAPI.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

class Project;
class GameEntity;

struct TransformData
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    bool operator==(const TransformData& other) const {
        return position == other.position &&
               rotation == other.rotation &&
               scale == other.scale;
    }

    bool operator!=(const TransformData& other) const {
        return !(*this == other);
    }
};

class TransformService
{
public:
    static TransformService& Get()
    {
        static TransformService instance;
        return instance;
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
        return value;
    }

    TransformData GetEntityTransform(uint32_t entityId)
    {
        TransformData data;
        transform_component comp{};

        float pos[3];
        float rot[3];
        float scl[3];

        if (::GetEntityTransform(entityId, &comp))
        {
            memcpy(pos, loadFromEngine(&comp), 3 * sizeof(float));
            memcpy(rot, loadFromEngine(&comp) + 3, 3 * sizeof(float));
            memcpy(scl, loadFromEngine(&comp) + 6, 3 * sizeof(float));
        }

        // Match the old working code exactly - no :: prefix
        data.position = glm::vec3(pos[0], pos[1], pos[2]);
        data.rotation = glm::vec3(rot[0], rot[1], rot[2]);
        data.scale = glm::vec3(scl[0], scl[1], scl[2]);

        return data;
    }

    bool SetEntityTransform(uint32_t entityId, const TransformData& data)
    {
        transform_component comp{};
        comp.position[0] = data.position.x;
        comp.position[1] = data.position.y;
        comp.position[2] = data.position.z;
        comp.rotation[0] = data.rotation.x;
        comp.rotation[1] = data.rotation.y;
        comp.rotation[2] = data.rotation.z;
        comp.scale[0] = data.scale.x;
        comp.scale[1] = data.scale.y;
        comp.scale[2] = data.scale.z;

        return ::SetEntityTransform(entityId, comp);
    }

    TransformData DecomposeMatrix(const float* matrix)
    {
        TransformData data;

        if (!matrix) return data;

        glm::mat4 transform = glm::make_mat4(matrix);
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat rotation;

        if (glm::decompose(transform, data.scale, rotation, data.position, skew, perspective)) {
            // Convert quaternion to euler angles in degrees
            data.rotation = glm::degrees(glm::eulerAngles(rotation));
        }

        return data;
    }

    glm::mat4 ComposeMatrix(const TransformData& data)
    {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), data.position);
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, glm::radians(data.rotation.x), glm::vec3(1, 0, 0));
        rotation = glm::rotate(rotation, glm::radians(data.rotation.y), glm::vec3(0, 1, 0));
        rotation = glm::rotate(rotation, glm::radians(data.rotation.z), glm::vec3(0, 0, 1));
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), data.scale);

        return translation * rotation * scale;
    }

    bool UpdateEntityTransform(std::shared_ptr<GameEntity> entity, const TransformData& data)
    {
        if (!entity) return false;

        // Update engine
        if (!SetEntityTransform(entity->GetID(), data)) {
            return false;
        }

        // Update component
        if (auto* transform = entity->GetComponent<Transform>()) {
            transform->SetPosition(data.position.x, data.position.y, data.position.z);
            transform->SetRotation(data.rotation.x, data.rotation.y, data.rotation.z);
            transform->SetScale(data.scale.x, data.scale.y, data.scale.z);
        }

        return true;
    }

    void BatchUpdateTransforms(const std::vector<std::shared_ptr<GameEntity>>& entities,
                              const std::function<TransformData(const TransformData&)>& transformer)
    {
        for (const auto& entity : entities) {
            if (!entity) continue;

            TransformData currentData = GetEntityTransform(entity->GetID());
            TransformData newData = transformer(currentData);
            UpdateEntityTransform(entity, newData);
        }
    }

private:
    TransformService() = default;
    ~TransformService() = default;
    TransformService(const TransformService&) = delete;
    TransformService& operator=(const TransformService&) = delete;
};