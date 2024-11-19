#pragma once
#include "Component.h"
#include <string>

class Script : public Component {
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

private:
    std::string m_scriptName;
};