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

std::unique_ptr<GeometryRenderer::LODGroupBuffers> GeometryRenderer::CreateBuffersFromGeometry(drosim::editor::Geometry* geometry) {
    if (!geometry) {
        printf("[CreateBuffersFromGeometry] Null geometry passed.\n");
        return nullptr;
    }

    auto groupBuffers = std::make_unique<LODGroupBuffers>();

    // Process each LOD group
    if (auto* lodGroup = geometry->GetLODGroup(0)) {
        groupBuffers->name = lodGroup->name;


        // Process each LOD level
        for (const auto& lod : lodGroup->lods) {
            auto lodBuffers = std::make_shared<LODLevelBuffers>();
            lodBuffers->name = lod->name;
            lodBuffers->threshold = lod->lod_threshold;

            // Process each mesh in this LOD level
            for (const auto& mesh : lod->meshes) {
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

std::shared_ptr<GeometryRenderer::MeshBuffers> GeometryRenderer::CreateMeshBuffers(const std::shared_ptr<drosim::editor::Mesh>& mesh) {
    if (!mesh || mesh->vertices.empty() || mesh->indices.empty()) return nullptr;

    printf("[CreateMeshBuffers] Creating buffers for mesh with:\n");
    printf("  Vertex count: %d, size: %d, total: %zu\n", mesh->vertex_count, mesh->vertex_size, mesh->vertices.size());
    printf("  Index count: %d, size: %d, total: %zu\n", mesh->index_count, mesh->index_size, mesh->indices.size());

    auto buffers = std::make_shared<MeshBuffers>();

    // Create and bind VAO
    glGenVertexArrays(1, &buffers->vao);
    glBindVertexArray(buffers->vao);

    // Create and bind VBO
    glGenBuffers(1, &buffers->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffers->vbo);
    
    // Calculate total vertex data size
    size_t vertexDataSize = static_cast<size_t>(mesh->vertex_count) * static_cast<size_t>(mesh->vertex_size);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize, mesh->vertices.data(), GL_STATIC_DRAW);

    // Create and bind IBO
    glGenBuffers(1, &buffers->ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers->ibo);
    
    // Calculate total index data size
    size_t indexDataSize = static_cast<size_t>(mesh->index_count) * static_cast<size_t>(mesh->index_size);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSize, mesh->indices.data(), GL_STATIC_DRAW);

    // Store index count and type
    buffers->indexCount = mesh->index_count;
    buffers->indexType = mesh->index_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

    // Set up vertex attributes based on vertex format
    size_t offset = 0;
    const int stride = mesh->vertex_size;

    // Position attribute (3 floats)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    offset += sizeof(float) * 3;

    // Normal attribute (3 floats)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
    offset += sizeof(float) * 3;

    // UV attribute (2 floats)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);

    // Unbind VAO
    glBindVertexArray(0);

    return buffers;
};

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
