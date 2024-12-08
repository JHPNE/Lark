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
  void SetLodGroup(drosim::editor::LODGroup l_group) {lod_group = l_group; };
  drosim::editor::LODGroup* GetLodGroup() {return &lod_group; };
  GeometryType GetGeometryType() const { return m_geometryType; };

  void loadGeometry() {
    float size[3] = {5.0f, 5.0f, 5.0f};
    uint32_t segments[3] = {32, 16, 1};

    auto geometry = m_geometryType == ObjImport
                ? drosim::editor::Geometry::LoadGeometry(m_geometrySource.c_str())
                : drosim::editor::Geometry::CreatePrimitive(
                        content_tools::PrimitiveMeshType::uv_sphere,
                        size,
                        segments
                    );
    auto lodGroup = geometry->GetLODGroup();
    if (lodGroup) {
      SetLodGroup(*lodGroup);
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
    }

    return true; // Return true if deserialization was successful
  }
private:
  std::string m_geometryName;
  bool visible = true;
  std::string m_geometrySource;
  GeometryType m_geometryType;
  drosim::editor::LODGroup lod_group;
};