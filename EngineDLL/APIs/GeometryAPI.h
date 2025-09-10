#pragma once
#include "EngineCoreAPI.h"
#include <LarkAPI/GeometryAPI.h>

#ifdef __cplusplus
extern "C"
{
#endif

    ENGINE_API bool CreatePrimitiveMesh(content_tools::SceneData *data,
                                        const content_tools::PrimitiveInitInfo *info);
    ENGINE_API bool LoadObj(const char *path, content_tools::SceneData *data);
    ENGINE_API bool ModifyEntityVertexPositions(lark::id::id_type entity_id,
                                                std::vector<glm::vec3> &new_positions);
    ENGINE_API bool GetModifiedMeshData(lark::id::id_type entity_id,
                                        content_tools::SceneData *data);

#ifdef __cplusplus
}
#endif
