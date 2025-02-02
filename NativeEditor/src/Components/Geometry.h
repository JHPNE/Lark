#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Geometry/Geometry.h"
#include "Utils/System/Serialization.h"
#include <string>

class Geometry : public Component, public ISerializable {
public:
  Geometry(GameEntity* owner)
      : Component(owner) {}

  ComponentType GetType() const override { return GetStaticType(); }
  static ComponentType GetStaticType() { return ComponentType::Geometry; }

  bool Initialize(const ComponentInitializer* init) override {
    if (init) {
      const auto* GeometryInit = static_cast<const GeometryInitializer*>(init);
      m_geometryName = GeometryInit->geometryName;
    }
    return true;
  }

  const std::string& GetGeometryName() const { return m_geometryName; }
  void SetGeometryName(const std::string &name) { m_geometryName = name; }
  bool IsVisible() {return visible; };
  void SetVisible(bool visible) { this->visible = visible; };
  void SetGeometrySource(const std::string &source) { m_geometrySource = source; };
  void SetGeometryType(GeometryType type) { m_geometryType = type; };
  void SetScene(lark::editor::scene scene) {m_scene = scene; };
  lark::editor::scene* GetScene() {return &m_scene; };
  GeometryType GetGeometryType() const { return m_geometryType; };

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
      geometry = m_geometryType == ObjImport
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
    auto GeometryNameElement = context.document.NewElement("GeometryName");
    SerializerUtils::WriteAttribute(GeometryNameElement, "GeometryName", m_geometryName.c_str());
    element->LinkEndChild(GeometryNameElement);

    auto visibleElement = context.document.NewElement("Visible");
    SerializerUtils::WriteAttribute(visibleElement, "Visible", visible);
    element->LinkEndChild(visibleElement);

    auto geometrySourceElement = context.document.NewElement("GeometrySource");
    SerializerUtils::WriteAttribute(geometrySourceElement, "GeometrySourceElement", m_geometrySource);
    SerializerUtils::WriteAttribute(geometrySourceElement, "GeometryType", m_geometryType == PrimitiveType ? "P" : "O");
    SerializerUtils::WriteAttribute(geometrySourceElement, "PrimitiveMeshType", m_meshType == content_tools::PrimitiveMeshType::cube ? "cube" : "uv_sphere");
    element->LinkEndChild(geometrySourceElement);
  }

  bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override {
    if (auto GeometryNameElement = element->FirstChildElement("GeometryName")) {
      m_geometryName = GeometryNameElement->Attribute("GeometryName");
    }

    if (auto VisibleElement = element->FirstChildElement("Visible")) {
      visible = VisibleElement->BoolAttribute("Visible");
    }

    if (auto GeometrySourceElement = element->FirstChildElement("GeometrySource")) {
      m_geometrySource = GeometrySourceElement->Attribute("GeometrySourceElement");

      const char* geometryTypeStr = GeometrySourceElement->Attribute("GeometryType");
      if (geometryTypeStr) {
        // Map string to GeometryType using a switch or a lookup function
        if (std::strcmp(geometryTypeStr, "PrimitiveType") == 0) {
          m_geometryType = GeometryType::PrimitiveType;
        } else if (std::strcmp(geometryTypeStr, "ObjImport") == 0) {
          m_geometryType = GeometryType::ObjImport;
        } else {
          // Handle invalid input
          throw std::invalid_argument("Invalid GeometryType attribute value");
        }
      }

      const char* geometryPrimitiveMeshTypeStr = GeometrySourceElement->Attribute("PrimitiveMeshType");
      if (std::strcmp(geometryPrimitiveMeshTypeStr, "cube") == 0) {
        m_meshType = content_tools::PrimitiveMeshType::cube;
      } else {
        m_meshType = content_tools::PrimitiveMeshType::uv_sphere;
      }
    }

    return true; // Return true if deserialization was successful
  }
private:
  std::string m_geometryName;
  bool visible = true;
  std::string m_geometrySource;
  GeometryType m_geometryType;
  lark::editor::scene m_scene;
  content_tools::PrimitiveMeshType m_meshType;
};