#pragma once
#include "Component.h"
#include <string>
#include "EngineAPI.h"

class Script : public Component {
public:
    Script(GameEntity* owner)
        : Component(owner, ComponentType::Script) {}

    const std::string& GetScriptName() const { return m_scriptName; }
    void SetScriptName(const std::string& name) { m_scriptName = name; }

private:
    std::string m_scriptName;
};