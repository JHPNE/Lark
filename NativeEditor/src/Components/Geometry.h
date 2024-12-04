#pragma once
#include "Component.h"
#include "EngineAPI.h"
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

  // Serialization interface
  void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override {
    auto GeometryNameElement = context.document.NewElement("GeometryName");
    GeometryNameElement->SetAttribute("Name", m_geometryName.c_str());
    element->LinkEndChild(GeometryNameElement);
  }
  bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override {
    if (auto GeometryNameElement = element->FirstChildElement("GeometryName")) {
      m_geometryName = GeometryNameElement->Attribute("Name");
    }
  }
private:
  std::string m_geometryName;
  bool visible = true;
};