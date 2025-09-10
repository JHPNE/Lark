#pragma once
#include "TransformAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C" {
    ENGINE_API bool SetEntityTransform(lark::id::id_type entity_id, const transform_component& transform) {
        if (!engine::is_entity_valid(entity_id)) return false;

        auto entity = engine::entity_from_id(entity_id);
        auto transform_comp = entity.transform();
        if (!transform_comp.is_valid()) return false;

        transform_comp.set_position(math::v3(transform.position[0], transform.position[1], transform.position[2]));

        math::v3 euler(transform.rotation[0], transform.rotation[1], transform.rotation[2]);
        glm::quat quat = glm::quat(glm::radians(euler));
        transform_comp.set_rotation(math::v4(quat.x, quat.y, quat.z, quat.w));

        transform_comp.set_scale(math::v3(transform.scale[0], transform.scale[1], transform.scale[2]));
        return true;
    }

    ENGINE_API bool GetEntityTransform(lark::id::id_type entity_id, transform_component* out_transform) {
        if (!engine::is_entity_valid(entity_id) || !out_transform) return false;

        auto entity = engine::entity_from_id(entity_id);
        auto transform_comp = entity.transform();
        if (!transform_comp.is_valid()) return false;

        math::v3 pos = transform_comp.position();
        math::v4 rot = transform_comp.rotation();
        math::v3 scale = transform_comp.scale();

        glm::quat quat(rot.w, rot.x, rot.y, rot.z);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(quat));

        memcpy(out_transform->position, &pos, sizeof(pos)*3);
        memcpy(out_transform->rotation, &euler, sizeof(rot)*3);
        memcpy(out_transform->scale, &scale, sizeof(scale)*3);
        return true;
    }

    ENGINE_API bool ResetEntityTransform(lark::id::id_type entity_id) {
        if (!engine::is_entity_valid(entity_id)) return false;

        auto entity = engine::entity_from_id(entity_id);
        auto transform_comp = entity.transform();
        if (!transform_comp.is_valid()) return false;
        transform_comp.reset();

        return true;
    }

    ENGINE_API glm::mat4 GetEntityTransformMatrix(lark::id::id_type entity_id) {
        if (!engine::is_entity_valid(entity_id)) return glm::mat4(1.0f);
        auto entity = engine::entity_from_id(entity_id);
        auto transform_comp = entity.transform();
        if (!transform_comp.is_valid()) return glm::mat4(1.0f);

        return transform_comp.get_transform_matrix();
    }
}