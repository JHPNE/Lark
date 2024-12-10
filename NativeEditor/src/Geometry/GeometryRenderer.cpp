#include "GeometryRenderer.h"

GLuint GeometryRenderer::m_basicShader = 0;

bool GeometryRenderer::Initialize() {
    // Create and compile shaders
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, s_basicVertexShader);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, s_basicFragmentShader);

    if (!vertexShader || !fragmentShader) return false;

    // Create shader program
    m_basicShader = glCreateProgram();
    glAttachShader(m_basicShader, vertexShader);
    glAttachShader(m_basicShader, fragmentShader);
    glLinkProgram(m_basicShader);

    // Check for linking errors
    GLint success;
    glGetProgramiv(m_basicShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_basicShader, 512, nullptr, infoLog);
        // Handle error
        return false;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
};

void GeometryRenderer::Shutdown() {
    if (m_basicShader) {
        glDeleteProgram(m_basicShader);
        m_basicShader = 0;
    }
}

std::unique_ptr<GeometryRenderer::LODGroupBuffers> GeometryRenderer::CreateBuffersFromGeometry(drosim::editor::scene* scene) {
    if (!scene) {
        printf("[CreateBuffersFromGeometry] Null geometry passed.\n");
        return nullptr;
    }

    auto groupBuffers = std::make_unique<LODGroupBuffers>();

    printf("[Application] Creating a new UvSphere");

    // Process each LOD group
    if (scene) {

        groupBuffers->name = scene->name;


        // Process each LOD level
        for (const auto& lod : scene->lod_groups) {
            auto lodBuffers = std::make_shared<LODLevelBuffers>();
            lodBuffers->name = lod.name;


            // Process each mesh in this LOD level
            for (const auto& mesh : lod.meshes) {
                lodBuffers->threshold = mesh.lod_threshold;
                auto meshBuffers = CreateMeshBuffers(mesh);
                if (meshBuffers) {
                    lodBuffers->meshBuffers.push_back(std::move(meshBuffers));
                }
            }

            groupBuffers->lodLevels.push_back(lodBuffers);
        }
    }

    return groupBuffers;
}

std::unique_ptr<GeometryRenderer::LODGroupBuffers> GeometryRenderer::UpdateBuffersfromGeometry(drosim::editor::scene* scene, std::unique_ptr<LODGroupBuffers> buffers) {
    if (!scene) {
        printf("[UpdateBuffersfromGeometry] Null geometry passed.\n");
        return nullptr;
    }
    if (scene) {

        buffers->name = scene->name;

        // Process each LOD level
        for (const auto& lod : scene->lod_groups) {
            auto lodBuffers = std::make_shared<LODLevelBuffers>();
            lodBuffers->name = lod.name;


            // Process each mesh in this LOD level
            for (const auto& mesh : lod.meshes) {
                lodBuffers->threshold = mesh.lod_threshold;
                auto meshBuffers = CreateMeshBuffers(mesh);
                if (meshBuffers) {
                    lodBuffers->meshBuffers.push_back(std::move(meshBuffers));
                }
            }

            buffers->lodLevels.push_back(lodBuffers);
        }
    }

    return buffers;
}

void GeometryRenderer::RenderGeometryAtLOD(const LODGroupBuffers* groupBuffers,
                             const glm::mat4& view,
                             const glm::mat4& projection,
                             float distanceToCamera) {

    if (!groupBuffers || groupBuffers->lodLevels.empty()) {
        printf("[RenderGeometryAtLOD] Invalid or empty LOD group buffers.\n");
        return;
    }

    // Use shader program
    glUseProgram(m_basicShader);

    // Set uniforms
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_basicShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Set object color (using a flat color without lighting)
    glm::vec3 faceColor(0.9f, 0.9f, 1.0f);  // Very light blue for faces
    glUniform3fv(glGetUniformLocation(m_basicShader, "objectColor"), 1, glm::value_ptr(faceColor));

    const LODLevelBuffers* selectedLOD = nullptr;

    for (const auto& lod : groupBuffers->lodLevels) {
        if (lod->threshold <= distanceToCamera) {
            selectedLOD = lod.get();
            break;
        }
    }

    // if no LOD level selected, use the last one
    if (!selectedLOD && !groupBuffers->lodLevels.empty()) {
        selectedLOD = groupBuffers->lodLevels.back().get();
    }

    if (!selectedLOD) {
        printf("[RenderGeometryAtLOD] No LOD level selected, rendering skipped.\n");
        return;
    }

    // Render all meshes in the selected LOD level
    if (selectedLOD) {
        for (const auto& meshBuffers : selectedLOD->meshBuffers) {
            RenderMesh(meshBuffers.get());
        }
    }
};

std::shared_ptr<GeometryRenderer::MeshBuffers> GeometryRenderer::CreateMeshBuffers(const drosim::editor::mesh& mesh) {
    if (mesh.vertices.empty() || mesh.indices.empty()) {
        printf("[CreateMeshBuffers] Empty mesh passed.\n");
        return nullptr;
    }

    auto buffers = std::make_shared<MeshBuffers>();

    glGenVertexArrays(1, &buffers->vao);
    glBindVertexArray(buffers->vao);

    glGenBuffers(1, &buffers->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffers->vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(drosim::editor::vertex), mesh.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &buffers->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), GL_STATIC_DRAW);

    size_t stride = sizeof(drosim::editor::vertex);

    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(drosim::editor::vertex, position));

    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(drosim::editor::vertex, normal));

    glEnableVertexAttribArray(2); // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(drosim::editor::vertex, uv));

    glBindVertexArray(0);

    buffers->indexCount = static_cast<GLsizei>(mesh.indices.size());
    buffers->indexType = GL_UNSIGNED_INT;

    return buffers;
}


void GeometryRenderer::RenderMesh(const MeshBuffers* meshBuffers) {
    if (!meshBuffers || !meshBuffers->vao) return;

    glBindVertexArray(meshBuffers->vao);
    
    // Draw filled geometry first
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUniform3f(glGetUniformLocation(m_basicShader, "objectColor"), 0.8f, 0.8f, 0.9f);  // Light blue-gray for faces
    glDrawElements(GL_TRIANGLES,
                  static_cast<GLsizei>(meshBuffers->indexCount),
                  meshBuffers->indexType,
                  0);

    // Draw edges
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.0f);
    glUniform3f(glGetUniformLocation(m_basicShader, "objectColor"), 0.2f, 0.2f, 0.3f);  // Dark blue-gray for edges
    
    // Enable polygon offset to prevent z-fighting
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    
    glDrawElements(GL_TRIANGLES,
                  static_cast<GLsizei>(meshBuffers->indexCount),
                  meshBuffers->indexType,
                  0);
    
    // Reset states
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}

GLuint GeometryRenderer::CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        return 0;
    }
    return shader;
}
