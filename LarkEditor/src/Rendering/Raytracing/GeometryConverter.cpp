#include "GeometryConverter.h"

std::vector<Triangle> GeometryConverter::ConvertFromGeometry(const content_tools::scene *geometry, uint32_t materialId){
    std::vector<Triangle> triangles;
    if (!geometry) return triangles;

    glm::mat4 transform = glm::mat4(1.0f);
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (const auto& lodGroup : geometry->lod_groups)
    {
        if (lodGroup.meshes.empty()) continue;

        const auto& mesh = lodGroup.meshes[0];

        for (size_t i = 0; i + 2 < mesh.indices.size(); i+= 3)
        {

            uint32_t i0 = mesh.indices[i];
            uint32_t i1 = mesh.indices[i + 1];
            uint32_t i2 = mesh.indices[i + 2];

            if (i0 >= mesh.vertices.size() ||
                i1 >= mesh.vertices.size() ||
                i2 >= mesh.vertices.size())
                continue;

            const auto& v0 = mesh.vertices[i0];
            const auto& v1 = mesh.vertices[i1];
            const auto& v2 = mesh.vertices[i2];

            Triangle tri;

            glm::vec4 p0 = transform * glm::vec4(v0.position.x, v0.position.y, v0.position.z, 1.0f);
            glm::vec4 p1 = transform * glm::vec4(v1.position.x, v1.position.y, v1.position.z, 1.0f);
            glm::vec4 p2 = transform * glm::vec4(v2.position.x, v2.position.y, v2.position.z, 1.0f);

            // Store positions as vec4 (w component is padding)
            tri.v0 = glm::vec4(glm::vec3(p0), 0.0f);
            tri.v1 = glm::vec4(glm::vec3(p1), 0.0f);
            tri.v2 = glm::vec4(glm::vec3(p2), 0.0f);

            // Transform normals and store as vec4
            glm::vec3 n0 = glm::normalize(normalMatrix * glm::vec3(v0.normal.x, v0.normal.y, v0.normal.z));
            glm::vec3 n1 = glm::normalize(normalMatrix * glm::vec3(v1.normal.x, v1.normal.y, v1.normal.z));
            glm::vec3 n2 = glm::normalize(normalMatrix * glm::vec3(v2.normal.x, v2.normal.y, v2.normal.z));
            
            tri.n0 = glm::vec4(n0, 0.0f);
            tri.n1 = glm::vec4(n1, 0.0f);
            tri.n2 = glm::vec4(n2, 0.0f);

            // Copy UVs
            tri.uv0 = glm::vec2(v0.uv.x, v0.uv.y);
            tri.uv1 = glm::vec2(v1.uv.x, v1.uv.y);
            tri.uv2 = glm::vec2(v2.uv.x, v2.uv.y);

            tri.materialId = materialId;
            tri._padding1 = 0.0f;

            triangles.push_back(tri);
        }
    }

    return triangles;
}
