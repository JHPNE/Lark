#include "GeometryImporter.h"

namespace drosim::tools {
    bool loadObj(const char* path, drosim::tools::scene_data* data){
      if (!path || !data) return false;
      std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
      std::vector<glm::vec3> temp_vertices;
      std::vector<glm::vec2> temp_uvs;
      std::vector<glm::vec3> temp_normals;

      FILE *file = fopen(path, "r");
      if (file == NULL){
        printf("Could not open file %s\n", path);
        return false;
      }

      while (true) {
        char lineHeader[128];

        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF){
          break;
        }

        if (strcmp(lineHeader, "v") == 0){
          glm::vec3 vertex;
          fscanf(file, "%f %f %f", &vertex.x, &vertex.y, &vertex.z);
          temp_vertices.push_back(vertex);
        } else if ( strcmp(lineHeader, "vt") == 0){
          glm::vec2 uv;
          fscanf(file, "%f %f", &uv.x, &uv.y);
          temp_uvs.push_back(uv);
        } else if ( strcmp(lineHeader, "vn") == 0){
          glm::vec3 normal;
          fscanf(file, "%f %f %f", &normal.x, &normal.y, &normal.z);
          temp_normals.push_back(normal);
        } else if ( strcmp(lineHeader, "f") == 0){
          std::string vertex1, vertex2, vertex3;
          unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
          int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
          if (matches != 9){
            printf("Could not read vertex coordinates from file %s\n", path);
            return false;
          }
          vertexIndices.push_back(vertexIndex[0]);
          vertexIndices.push_back(vertexIndex[1]);
          vertexIndices.push_back(vertexIndex[2]);
          uvIndices.push_back(uvIndex[0]);
          uvIndices.push_back(uvIndex[1]);
          uvIndices.push_back(uvIndex[2]);
          normalIndices.push_back(normalIndex[0]);
          normalIndices.push_back(normalIndex[1]);
          normalIndices.push_back(normalIndex[2]);
        }

      }

        // Create mesh from the loaded data
        drosim::tools::mesh obj_mesh{};
        obj_mesh.name = path;

        // Store vertices and create proper indices
        obj_mesh.positions = temp_vertices;
        obj_mesh.raw_indices.reserve(vertexIndices.size());
        for (size_t i = 0; i < vertexIndices.size(); i++) {
            obj_mesh.raw_indices.push_back(vertexIndices[i] - 1);
        }

        // Store UVs if present
        if (!temp_uvs.empty()) {
            obj_mesh.uv_sets.resize(1);
            obj_mesh.uv_sets[0] = temp_uvs;
        }

        // Store normals if present, otherwise they'll be calculated
        if (!temp_normals.empty()) {
            obj_mesh.normals = temp_normals;
        }

        // Create scene with proper LOD structure
        drosim::tools::scene scene{};
        scene.name = path;

        drosim::tools::lod_group lod{};
        lod.name = path;
        lod.meshes.push_back(std::move(obj_mesh));
        scene.lod_groups.push_back(std::move(lod));

        // Configure processing settings
        data->settings.calculate_normals = data->settings.calculate_normals || temp_normals.empty();
        // Use default smoothing angle from primitive meshes
        data->settings.smoothing_angle = 178.0f;

        // Process scene (this handles normal calculation and smoothing)
        drosim::tools::process_scene(scene, data->settings);

        // Pack the processed data
        drosim::tools::pack_data(scene, *data);

        fclose(file);

        return (data->buffer && data->buffer_size > 0);
    };
}