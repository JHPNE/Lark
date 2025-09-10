#include "GeometryImporter.h"

#include <array>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>

namespace lark::tools
{
namespace
{
// Fast string to float conversion without locale overhead
inline float fast_strtof(const char *str, const char **str_end)
{
    char *end;
    float result = std::strtof(str, &end);
    if (str_end)
        *str_end = end;
    return result;
}

// Fast string to int conversion
inline int fast_parse_int(const char *str, const char **str_end)
{
    int result;
    auto [ptr, ec] = std::from_chars(str, str + 32, result);
    if (str_end)
        *str_end = ptr;
    return result;
}

// Parse face indices
bool parse_face_indices(const std::string_view &token, unsigned int &v, unsigned int &t,
                        unsigned int &n)
{
    const char *start = token.data();
    const char *end = start + token.size();
    const char *current = start;

    // Parse vertex index
    v = fast_parse_int(current, &current);
    if (current >= end || *current != '/')
        return false;
    current++; // Skip '/'

    // Parse texture index
    if (*current != '/')
    {
        t = fast_parse_int(current, &current);
        if (current >= end || *current != '/')
            return false;
    }
    current++; // Skip '/'

    // Parse normal index
    if (current < end)
    {
        n = fast_parse_int(current, nullptr);
    }

    return true;
}
} // namespace

bool loadObj(const char *path, scene_data *data)
{
    if (!path || !data)
        return false;

    // Use RAII file handling
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return false;

    // Pre-allocate buffer based on file size
    const size_t file_size = file.tellg();
    file.seekg(0);

    // Rough estimation of required capacity
    const size_t estimated_vertices = file_size / 100; // Approximate ratio
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(estimated_vertices);
    indices.reserve(estimated_vertices * 2);

    std::string line;
    line.reserve(128); // Pre-allocate line buffer

    // Temporary buffers for parsing
    std::array<unsigned int, 12> face_data;
    char *line_ptr;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        switch (line[0])
        {
        case 'v':
        {
            if (line[1] != ' ')
                continue;
            glm::vec3 vertex;
            const char *line_ptr = line.data() + 2;
            const char *next_ptr;

            vertex.x = fast_strtof(line_ptr, &next_ptr);
            line_ptr = next_ptr;
            vertex.y = fast_strtof(line_ptr, &next_ptr);
            line_ptr = next_ptr;
            vertex.z = fast_strtof(line_ptr, nullptr); // Last one doesn't need next pointer

            vertices.push_back(vertex);
            break;
        }
        case 'f':
        {
            if (line[1] != ' ')
                continue;

            // Split the face line into tokens
            std::stringstream ss(line.substr(2));
            std::string token;
            std::vector<unsigned int> face_vertices;
            face_vertices.reserve(4); // Most faces are quads or triangles

            while (ss >> token)
            {
                unsigned int v = 0, t = 0, n = 0;
                if (parse_face_indices(token, v, t, n))
                {
                    face_vertices.push_back(v - 1); // OBJ indices are 1-based
                }
            }

            // Triangulate the face
            for (size_t i = 2; i < face_vertices.size(); ++i)
            {
                indices.push_back(face_vertices[0]);
                indices.push_back(face_vertices[i - 1]);
                indices.push_back(face_vertices[i]);
            }
            break;
        }
        }
    }

    // Create mesh and scene
    mesh obj_mesh{};
    obj_mesh.name = path;
    obj_mesh.positions = std::move(vertices);
    obj_mesh.raw_indices = std::move(indices);

    scene scene{};
    scene.name = path;

    lod_group lod{};
    lod.name = path;
    lod.meshes.push_back(std::move(obj_mesh));
    scene.lod_groups.push_back(std::move(lod));

    // Process and pack data
    data->settings.calculate_normals = true;
    data->settings.smoothing_angle = 178.0f;

    process_scene(scene, data->settings);
    pack_data(scene, *data);

    return (data->buffer && data->buffer_size > 0);
}
} // namespace lark::tools