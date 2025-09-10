#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Geometry/Geometry.h"
#include "Utils/System/Serialization.h"
#include <string>

class Geometry : public Component, public ISerializable
{
  public:
    Geometry(GameEntity *owner) : Component(owner) {}

    ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Geometry; }

    bool Initialize(const ComponentInitializer *init) override
    {
        if (init)
        {
            const auto *GeometryInit = static_cast<const GeometryInitializer *>(init);
            m_geometryName = GeometryInit->geometryName;
            m_geometryType = GeometryInit->geometryType;
            visible = GeometryInit->visible;
            m_geometrySource = GeometryInit->geometrySource;
            m_meshType = GeometryInit->meshType;
        }
        return true;
    }

    const std::string &GetGeometryName() const { return m_geometryName; }
    void SetGeometryName(const std::string &name) { m_geometryName = name; }
    bool IsVisible() { return visible; };
    void SetVisible(bool visible) { this->visible = visible; };
    void SetGeometrySource(const std::string &source) { m_geometrySource = source; };
    std::string GetGeometrySource() const { return m_geometrySource; };
    void SetGeometryType(GeometryType type) { m_geometryType = type; };
    void SetScene(content_tools::scene scene) { m_scene = scene; };
    content_tools::scene *GetScene() { return &m_scene; };
    GeometryType GetGeometryType() const { return m_geometryType; };
    content_tools::PrimitiveMeshType GetPrimitiveMeshType() const { return m_meshType; };

    void SetPrimitiveType(content_tools::PrimitiveMeshType type) { m_meshType = type; }

    void HandleVerticeSerialization(const content_tools::mesh &meshes,
                                    tinyxml2::XMLElement *meshesElement,
                                    const SerializationContext &context) const
    {
        // handle vertices
        for (const auto &vertex : meshes.vertices)
        {
            auto vertexElement = context.document.NewElement("Vertex");

            // Vertex Normal
            auto vertexElementNormal = context.document.NewElement("Normal");
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.b", vertex.normal.b);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.g", vertex.normal.g);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.p", vertex.normal.p);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.r", vertex.normal.r);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.s", vertex.normal.s);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.t", vertex.normal.t);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.x", vertex.normal.x);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.y", vertex.normal.y);
            SerializerUtils::WriteAttribute(vertexElementNormal, "normal.z", vertex.normal.z);

            vertexElement->LinkEndChild(vertexElementNormal);

            // Vertex Position
            auto vertexElementPosition = context.document.NewElement("Position");
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.b", vertex.position.b);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.g", vertex.position.g);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.p", vertex.position.p);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.r", vertex.position.r);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.s", vertex.position.s);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.t", vertex.position.t);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.x", vertex.position.x);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.y", vertex.position.y);
            SerializerUtils::WriteAttribute(vertexElementPosition, "position.z", vertex.position.z);

            vertexElement->LinkEndChild(vertexElementPosition);

            // Vertex Tangent
            auto vertexElementTangent = context.document.NewElement("Tangent");
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.a", vertex.tangent.a);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.b", vertex.tangent.b);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.g", vertex.tangent.g);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.p", vertex.tangent.p);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.q", vertex.tangent.q);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.r", vertex.tangent.r);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.s", vertex.tangent.s);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.t", vertex.tangent.t);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.w", vertex.tangent.w);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.x", vertex.tangent.x);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.y", vertex.tangent.y);
            SerializerUtils::WriteAttribute(vertexElementTangent, "tangent.z", vertex.tangent.z);

            vertexElement->LinkEndChild(vertexElementTangent);

            // Uvs
            auto vertexElementUVs = context.document.NewElement("UVs");
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.g", vertex.uv.g);
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.r", vertex.uv.r);
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.s", vertex.uv.s);
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.t", vertex.uv.t);
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.x", vertex.uv.x);
            SerializerUtils::WriteAttribute(vertexElementUVs, "uv.y", vertex.uv.y);

            vertexElement->LinkEndChild(vertexElementUVs);

            meshesElement->LinkEndChild(vertexElement);
        }
    };

    void HandleMeshPositionSerialization(const content_tools::mesh &mesh,
                                         tinyxml2::XMLElement *meshElement,
                                         const SerializationContext &context) const
    {
        auto meshPositions = context.document.NewElement("MeshPositions");
        for (auto position : mesh.positions)
        {
            SerializerUtils::WriteAttribute(meshPositions, "position.b", position.b);
            SerializerUtils::WriteAttribute(meshPositions, "position.g", position.g);
            SerializerUtils::WriteAttribute(meshPositions, "position.p", position.p);
            SerializerUtils::WriteAttribute(meshPositions, "position.r", position.r);
            SerializerUtils::WriteAttribute(meshPositions, "position.s", position.s);
            SerializerUtils::WriteAttribute(meshPositions, "position.t", position.t);
            SerializerUtils::WriteAttribute(meshPositions, "position.x", position.x);
            SerializerUtils::WriteAttribute(meshPositions, "position.y", position.y);
            SerializerUtils::WriteAttribute(meshPositions, "position.z", position.z);
        }
        meshElement->LinkEndChild(meshPositions);
    };

    void HandleMeshNormalSerialization(const content_tools::mesh &mesh,
                                       tinyxml2::XMLElement *meshElement,
                                       const SerializationContext &context) const
    {
        auto meshNormals = context.document.NewElement("MeshNormal");
        for (auto normal : mesh.normals)
        {
            SerializerUtils::WriteAttribute(meshNormals, "normal.b", normal.b);
            SerializerUtils::WriteAttribute(meshNormals, "normal.g", normal.g);
            SerializerUtils::WriteAttribute(meshNormals, "normal.p", normal.p);
            SerializerUtils::WriteAttribute(meshNormals, "normal.r", normal.r);
            SerializerUtils::WriteAttribute(meshNormals, "normal.s", normal.s);
            SerializerUtils::WriteAttribute(meshNormals, "normal.t", normal.t);
            SerializerUtils::WriteAttribute(meshNormals, "normal.x", normal.x);
            SerializerUtils::WriteAttribute(meshNormals, "normal.y", normal.y);
            SerializerUtils::WriteAttribute(meshNormals, "normal.z", normal.z);
        }
        meshElement->LinkEndChild(meshNormals);
    };

    // Serialization interface
    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
    {
        WriteVersion(element);

        // Basic properties
        SERIALIZE_PROPERTY(element, context, m_geometryName);
        SERIALIZE_PROPERTY(element, context, visible);

        // Always serialize the scene data if we have it
        if (!m_scene.lod_groups.empty())
        {
            auto sceneElement = context.document.NewElement("SceneData");
            SerializerUtils::WriteAttribute(sceneElement, "name", m_scene.name);

            auto lodGroupsElement = context.document.NewElement("LODGroups");

            for (const auto &lod_group : m_scene.lod_groups)
            {
                auto lodGroupElement = context.document.NewElement("LODGroup");
                SerializerUtils::WriteAttribute(lodGroupElement, "name", lod_group.name);

                auto meshesElement = context.document.NewElement("Meshes");

                for (const auto &mesh : lod_group.meshes)
                {
                    SerializeMesh(mesh, meshesElement, context);
                }

                lodGroupElement->LinkEndChild(meshesElement);
                lodGroupsElement->LinkEndChild(lodGroupElement);
            }

            sceneElement->LinkEndChild(lodGroupsElement);
            element->LinkEndChild(sceneElement);

            // Also save original source info as metadata (optional, for reference)
            if (!m_geometrySource.empty())
            {
                SerializerUtils::WriteAttribute(element, "originalSource", m_geometrySource);
            }
            std::string typeStr =
                (m_geometryType == GeometryType::PrimitiveType) ? "Primitive" : "ObjImport";
            SerializerUtils::WriteAttribute(element, "originalType", typeStr);

            if (m_geometryType == GeometryType::PrimitiveType)
            {
                std::string meshTypeStr =
                    (m_meshType == content_tools::PrimitiveMeshType::cube) ? "cube" : "uv_sphere";
                SerializerUtils::WriteAttribute(element, "primitiveType", meshTypeStr);
            }
        }
        else
        {
            // Fallback: if no scene data, save the source info to regenerate
            SERIALIZE_PROPERTY(element, context, m_geometrySource);
            std::string typeStr =
                (m_geometryType == GeometryType::PrimitiveType) ? "Primitive" : "ObjImport";
            SerializerUtils::WriteAttribute(element, "geometryType", typeStr);
            std::string meshTypeStr =
                (m_meshType == content_tools::PrimitiveMeshType::cube) ? "cube" : "uv_sphere";
            SerializerUtils::WriteAttribute(element, "primitiveType", meshTypeStr);
        }
    }

    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {
        context.version = ReadVersion(element);
        if (!SupportsVersion(context.version))
        {
            context.AddError("Unsupported version " + context.version.toString());
            return false;
        }

        // Basic properties
        DESERIALIZE_PROPERTY(element, context, m_geometryName);
        DESERIALIZE_PROPERTY(element, context, visible);

        // Try to deserialize scene data first
        // Deserialize scene data if present
        auto sceneElement = element->FirstChildElement("SceneData");
        if (sceneElement)
        {
            SerializerUtils::ReadAttribute(sceneElement, "name", m_scene.name);

            auto lodGroupsElement = sceneElement->FirstChildElement("LODGroups");
            if (lodGroupsElement)
            {
                m_scene.lod_groups.clear();

                for (auto lodGroupElement = lodGroupsElement->FirstChildElement("LODGroup");
                     lodGroupElement;
                     lodGroupElement = lodGroupElement->NextSiblingElement("LODGroup"))
                {

                    content_tools::lod_group lod_group;
                    SerializerUtils::ReadAttribute(lodGroupElement, "name", lod_group.name);

                    auto meshesElement = lodGroupElement->FirstChildElement("Meshes");
                    if (meshesElement)
                    {
                        for (auto meshElement = meshesElement->FirstChildElement("Mesh");
                             meshElement; meshElement = meshElement->NextSiblingElement("Mesh"))
                        {

                            content_tools::mesh mesh;
                            if (DeserializeMesh(mesh, meshElement, context))
                            {
                                lod_group.meshes.push_back(std::move(mesh));
                            }
                        }
                    }

                    m_scene.lod_groups.push_back(std::move(lod_group));
                }
            }
        }

        return !context.HasErrors();
    }

    Version GetVersion() const override { return {1, 1, 0}; };

  private:
    std::string m_geometryName;
    bool visible = true;
    std::string m_geometrySource;
    GeometryType m_geometryType;
    content_tools::scene m_scene;
    content_tools::PrimitiveMeshType m_meshType;

    void SerializeMesh(const content_tools::mesh &mesh, tinyxml2::XMLElement *parentElement,
                       SerializationContext &context) const
    {
        auto meshElement = context.document.NewElement("Mesh");
        SerializerUtils::WriteAttribute(meshElement, "name", mesh.name);
        SerializerUtils::WriteAttribute(meshElement, "lod_id", mesh.lod_id);
        SerializerUtils::WriteAttribute(meshElement, "lod_threshold", mesh.lod_threshold);

        // Serialize vertices (compact format)
        if (!mesh.vertices.empty())
        {
            auto verticesElement = context.document.NewElement("Vertices");
            SerializerUtils::WriteAttribute(verticesElement, "count",
                                            (uint32_t)mesh.vertices.size());

            for (size_t i = 0; i < mesh.vertices.size(); ++i)
            {
                const auto &v = mesh.vertices[i];
                auto vElement = context.document.NewElement("V");
                SerializerUtils::WriteAttribute(vElement, "i", (uint32_t)i);

                // Position
                SerializerUtils::WriteAttribute(vElement, "px", v.position.x);
                SerializerUtils::WriteAttribute(vElement, "py", v.position.y);
                SerializerUtils::WriteAttribute(vElement, "pz", v.position.z);

                // Normal
                SerializerUtils::WriteAttribute(vElement, "nx", v.normal.x);
                SerializerUtils::WriteAttribute(vElement, "ny", v.normal.y);
                SerializerUtils::WriteAttribute(vElement, "nz", v.normal.z);

                // Tangent
                SerializerUtils::WriteAttribute(vElement, "tx", v.tangent.x);
                SerializerUtils::WriteAttribute(vElement, "ty", v.tangent.y);
                SerializerUtils::WriteAttribute(vElement, "tz", v.tangent.z);
                SerializerUtils::WriteAttribute(vElement, "tw", v.tangent.w);

                // UV
                SerializerUtils::WriteAttribute(vElement, "u", v.uv.x);
                SerializerUtils::WriteAttribute(vElement, "v", v.uv.y);

                verticesElement->LinkEndChild(vElement);
            }
            meshElement->LinkEndChild(verticesElement);
        }

        // Serialize indices
        if (!mesh.indices.empty())
        {
            auto indicesElement = context.document.NewElement("Indices");
            SerializerUtils::WriteAttribute(indicesElement, "count", (uint32_t)mesh.indices.size());

            // Store indices as a compact string
            std::string indexStr;
            for (size_t i = 0; i < mesh.indices.size(); ++i)
            {
                if (i > 0)
                    indexStr += ",";
                indexStr += std::to_string(mesh.indices[i]);
            }
            indicesElement->SetText(indexStr.c_str());
            meshElement->LinkEndChild(indicesElement);
        }

        parentElement->LinkEndChild(meshElement);
    }

    bool DeserializeMesh(content_tools::mesh &mesh, const tinyxml2::XMLElement *meshElement,
                         SerializationContext &context) const
    {
        SerializerUtils::ReadAttribute(meshElement, "name", mesh.name);
        SerializerUtils::ReadAttribute(meshElement, "lod_id", mesh.lod_id);
        SerializerUtils::ReadAttribute(meshElement, "lod_threshold", mesh.lod_threshold);

        // Deserialize vertices
        auto verticesElement = meshElement->FirstChildElement("Vertices");
        if (verticesElement)
        {
            uint32_t count = 0;
            SerializerUtils::ReadAttribute(verticesElement, "count", count);

            mesh.vertices.reserve(count);
            mesh.positions.reserve(count);
            mesh.normals.reserve(count);
            mesh.tangents.reserve(count);

            for (auto vElement = verticesElement->FirstChildElement("V"); vElement;
                 vElement = vElement->NextSiblingElement("V"))
            {

                content_tools::vertex v;

                // Position
                SerializerUtils::ReadAttribute(vElement, "px", v.position.x);
                SerializerUtils::ReadAttribute(vElement, "py", v.position.y);
                SerializerUtils::ReadAttribute(vElement, "pz", v.position.z);

                // Normal
                SerializerUtils::ReadAttribute(vElement, "nx", v.normal.x);
                SerializerUtils::ReadAttribute(vElement, "ny", v.normal.y);
                SerializerUtils::ReadAttribute(vElement, "nz", v.normal.z);

                // Tangent
                SerializerUtils::ReadAttribute(vElement, "tx", v.tangent.x);
                SerializerUtils::ReadAttribute(vElement, "ty", v.tangent.y);
                SerializerUtils::ReadAttribute(vElement, "tz", v.tangent.z);
                SerializerUtils::ReadAttribute(vElement, "tw", v.tangent.w);

                // UV
                SerializerUtils::ReadAttribute(vElement, "u", v.uv.x);
                SerializerUtils::ReadAttribute(vElement, "v", v.uv.y);

                mesh.vertices.push_back(v);
                mesh.positions.push_back(v.position);
                mesh.normals.push_back(v.normal);
                mesh.tangents.push_back(v.tangent);

                // Handle UV sets
                if (mesh.uv_sets.empty())
                {
                    mesh.uv_sets.resize(1);
                }
                mesh.uv_sets[0].push_back(v.uv);
            }
        }

        // Deserialize indices
        auto indicesElement = meshElement->FirstChildElement("Indices");
        if (indicesElement)
        {
            uint32_t count = 0;
            SerializerUtils::ReadAttribute(indicesElement, "count", count);

            const char *indexText = indicesElement->GetText();
            if (indexText)
            {
                mesh.indices.reserve(count);
                std::string indexStr(indexText);
                size_t pos = 0;
                while (pos < indexStr.length())
                {
                    size_t nextPos = indexStr.find(',', pos);
                    if (nextPos == std::string::npos)
                        nextPos = indexStr.length();

                    std::string numStr = indexStr.substr(pos, nextPos - pos);
                    mesh.indices.push_back(std::stoul(numStr));

                    pos = nextPos + 1;
                }
            }
        }

        return true;
    }
};