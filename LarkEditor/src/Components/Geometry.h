#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Geometry/Geometry.h"
#include "Utils/System/Serialization.h"
#include <string>

class Geometry : public Component, public ISerializable {
public:
  Geometry(GameEntity* owner)
      : Component(owner){}

  ComponentType GetType() const override { return GetStaticType(); }
  static ComponentType GetStaticType() { return ComponentType::Geometry; }

  bool Initialize(const ComponentInitializer* init) override {
    if (init) {
      const auto* GeometryInit = static_cast<const GeometryInitializer*>(init);
      m_geometryName = GeometryInit->geometryName;
      m_geometryType = GeometryInit->geometryType;
      visible = GeometryInit->visible;
      m_geometrySource = GeometryInit->geometrySource;
      m_meshType = GeometryInit->meshType;
    }
    return true;
  }

  const std::string& GetGeometryName() const { return m_geometryName; }
  void SetGeometryName(const std::string &name) { m_geometryName = name; }
  bool IsVisible() {return visible; };
  void SetVisible(bool visible) { this->visible = visible; };
  void SetGeometrySource(const std::string &source) { m_geometrySource = source; };
  std::string GetGeometrySource() const { return m_geometrySource; };
  void SetGeometryType(GeometryType type) { m_geometryType = type; };
  void SetScene(lark::editor::scene scene) {m_scene = scene; };
  lark::editor::scene* GetScene() {return &m_scene; };
  GeometryType GetGeometryType() const { return m_geometryType; };
  content_tools::PrimitiveMeshType GetPrimitiveMeshType() const { return m_meshType; };

  void SetPrimitiveType(content_tools::PrimitiveMeshType type) {
    m_meshType = type;
  }

  void loadGeometry() {
    float size[3] = {5.0f, 5.0f, 5.0f};
    uint32_t segments[3];
    if (m_meshType == content_tools::PrimitiveMeshType::uv_sphere) {
      segments[0] = 32;
      segments[1] = 16;
      segments[2] = 1;
    } else if (m_meshType == content_tools::PrimitiveMeshType::cube ) {
      segments[0] = segments[1] = segments[2] = 16;
    } else {
      segments[0] = 32;
      segments[1] = 1;
    }

    std::shared_ptr<lark::editor::Geometry> geometry;
    try {
      geometry = false
                  ? lark::editor::Geometry::LoadGeometry(m_geometrySource.c_str())
                  : lark::editor::Geometry::CreatePrimitive(
                          m_meshType,
                          size,
                          segments
                      );

      if (!geometry) {
        printf("[Geometry::loadGeometry] Failed to create geometry\n");
        return;
      }

      auto scene = geometry->GetScene();
      if (scene) {
        SetScene(*scene);
      } else {
        printf("[Geometry::loadGeometry] No LOD group found in geometry\n");
      }
    } catch (const std::exception& e) {
      printf("[Geometry::loadGeometry] Exception while loading geometry: %s\n", e.what());
    }
  }


  // Serialization interface
  void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override {
    WriteVersion(element);

    SERIALIZE_PROPERTY(element, context, m_geometryName);
    SERIALIZE_PROPERTY(element, context, visible);
    SERIALIZE_PROPERTY(element, context, m_geometrySource);

    std::string typeStr = (m_geometryType == GeometryType::PrimitiveType) ? "Primitive" : "ObjImport";
    SerializerUtils::WriteAttribute(element, "geometryType", typeStr);

    std::string meshTypeStr = (m_meshType == content_tools::PrimitiveMeshType::cube) ? "cube" : "uv_sphere";
    SerializerUtils::WriteAttribute(element, "primitiveType", meshTypeStr);

  }

  bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override {
    context.version = ReadVersion(element);
    if (!SupportsVersion(context.version)) {
      context.AddError("Unsupported version " + context.version.toString());
      return false;
    }

    DESERIALIZE_PROPERTY(element, context, m_geometryName);
    DESERIALIZE_PROPERTY(element, context, visible);
    DESERIALIZE_PROPERTY(element, context, m_geometrySource);

    std::string typeStr;
    if (SerializerUtils::ReadAttribute(element, "geometryType", typeStr)) {
      m_geometryType = (typeStr == "Primitive") ? GeometryType::PrimitiveType : GeometryType::ObjImport;
    }

    std::string primitiveType;
    if (SerializerUtils::ReadAttribute(element, "primitiveType", primitiveType)) {
      m_meshType = (primitiveType == "cube") ? content_tools::PrimitiveMeshType::cube : content_tools::PrimitiveMeshType::uv_sphere;
    }

    return !context.HasErrors();
  }

  Version GetVersion() const override { return { 1, 1, 0};};

private:
  std::string m_geometryName;
  bool visible = true;
  std::string m_geometrySource;
  GeometryType m_geometryType;
  lark::editor::scene m_scene;
  content_tools::PrimitiveMeshType m_meshType;
};