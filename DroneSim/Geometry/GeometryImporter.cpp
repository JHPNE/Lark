#include "GeometryImporter.h"

namespace drosim::tools {
    bool loadObj(const char* path, drosim::tools::scene_data* data) {
    if (!path || !data) return false;
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    FILE* file = fopen(path, "r");
    if (!file) {
        printf("Could not open file %s\n", path);
        return false;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character if present
        char* ptr = strchr(line, '\n');
        if (ptr) *ptr = '\0';

        if (line[0] == 'v' && line[1] == ' ') {
            glm::vec3 vertex;
            if (sscanf(line + 2, "%f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3) {
                temp_vertices.push_back(vertex);
                printf("Loaded vertex: %f %f %f\n", vertex.x, vertex.y, vertex.z);
            }
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            unsigned int vertexIndex[4], uvIndex[4], normalIndex[4];
            int matches = sscanf(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2],
                &vertexIndex[3], &uvIndex[3], &normalIndex[3]);

            // Triangle face
            if (matches >= 9) {
                for (int i = 0; i < 3; i++) {
                    vertexIndices.push_back(vertexIndex[i] - 1);
                }
                printf("Added triangle face: %d %d %d\n",
                    vertexIndex[0] - 1, vertexIndex[1] - 1, vertexIndex[2] - 1);

                // If we have a quad, add another triangle
                if (matches == 12) {
                    vertexIndices.push_back(vertexIndex[0] - 1);
                    vertexIndices.push_back(vertexIndex[2] - 1);
                    vertexIndices.push_back(vertexIndex[3] - 1);
                    printf("Added quad face part 2: %d %d %d\n",
                        vertexIndex[0] - 1, vertexIndex[2] - 1, vertexIndex[3] - 1);
                }
            }
        }
    }

    printf("Loading complete: %zu vertices, %zu indices\n",
        temp_vertices.size(), vertexIndices.size());

    drosim::tools::mesh obj_mesh{};
    obj_mesh.name = path;
    obj_mesh.positions = temp_vertices;
    obj_mesh.raw_indices = vertexIndices;

    drosim::tools::scene scene{};
    scene.name = path;

    drosim::tools::lod_group lod{};
    lod.name = path;
    lod.meshes.push_back(std::move(obj_mesh));
    scene.lod_groups.push_back(std::move(lod));

    // Force normal calculation since we're not loading them
    data->settings.calculate_normals = true;
    data->settings.smoothing_angle = 178.0f;

    drosim::tools::process_scene(scene, data->settings);
    drosim::tools::pack_data(scene, *data);

    fclose(file);
    return (data->buffer && data->buffer_size > 0);
}
}