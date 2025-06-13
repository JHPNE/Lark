#pragma once
#include "Component.h"
#include <string>

#include "EngineAPI.h"

class Script : public Component, public ISerializable {
public:
    Script(GameEntity* owner)
        : Component(owner) {}

    ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Script; }

    bool Initialize(const ComponentInitializer* init) override {
        if (init) {
            const auto* scriptInit = static_cast<const ScriptInitializer*>(init);
            m_scriptName = scriptInit->scriptName;
        }
        return true;
    }

    const std::string& GetScriptName() const { return m_scriptName; }
    void SetScriptName(const std::string& name) { m_scriptName = name; }

    // Serialization interface
    void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const override {
        auto scriptNameElement = context.document.NewElement("ScriptName");
        scriptNameElement->SetAttribute("Name", m_scriptName.c_str());
        element->LinkEndChild(scriptNameElement);
    }
    bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) override {
        if (auto scriptNameElement = element->FirstChildElement("ScriptName")) {
            m_scriptName = scriptNameElement->Attribute("Name");
        }
    }
private:
    std::string m_scriptName;
};
