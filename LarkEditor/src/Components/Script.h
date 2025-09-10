#pragma once
#include "Component.h"
#include "Utils/System/Serialization.h"
#include <string>

class Script : public Component, public ISerializable
{
  public:
    Script(GameEntity *owner) : Component(owner) {}

    ComponentType GetType() const override { return GetStaticType(); }
    static ComponentType GetStaticType() { return ComponentType::Script; }

    bool Initialize(const ComponentInitializer *init) override
    {
        if (init)
        {
            const auto *scriptInit = static_cast<const ScriptInitializer *>(init);
            m_scriptName = scriptInit->scriptName;
        }
        return true;
    }

    const std::string &GetScriptName() const { return m_scriptName; }
    void SetScriptName(const std::string &name) { m_scriptName = name; }

    // Serialization interface
    void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
    {
        WriteVersion(element);
        SERIALIZE_PROPERTY(element, context, m_scriptName);
    }
    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {
        context.version = ReadVersion(element);
        if (!SupportsVersion(context.version))
        {
            context.AddError("Unsupported Script version: " + context.version.toString());
            return false;
        }

        DESERIALIZE_PROPERTY(element, context, m_scriptName);
        return !context.HasErrors();
    }

    Version GetVersion() const override { return {1, 1, 0}; }

  private:
    std::string m_scriptName;
};
