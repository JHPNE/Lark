#pragma once
#include "tinyxml2.h"
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

struct Version
{
    uint32_t major = 1;
    uint32_t minor = 0;
    uint32_t patch = 0;

    bool operator>=(const Version &other) const
    {
        if (major != other.major)
            return major > other.major;
        if (minor != other.minor)
            return minor > other.minor;
        return patch >= other.patch;
    }

    std::string toString() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    static Version fromString(const std::string &str)
    {
        Version v;
        sscanf(str.c_str(), "%u.%u.%u", &v.major, &v.minor, &v.patch);
        return v;
    }
};

class SerializationContext
{
  public:
    explicit SerializationContext(tinyxml2::XMLDocument &doc) : document(doc), version{1, 0, 0} {}

    tinyxml2::XMLDocument &document;
    Version version;
    std::unordered_map<std::string, std::string> userData;

    // Error handling
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void AddError(const std::string &error) { errors.push_back(error); }
    void AddWarning(const std::string &warning) { warnings.push_back(warning); }
    bool HasErrors() const { return !errors.empty(); }
};

class ISerializable
{
  public:
    virtual ~ISerializable() = default;
    virtual void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const = 0;
    virtual bool Deserialize(const tinyxml2::XMLElement *element,
                             SerializationContext &context) = 0;

    // Optional: Override for versioning support
    virtual Version GetVersion() const { return {1, 0, 0}; }
    virtual bool SupportsVersion(const Version &version) const
    {
        return version >= Version{1, 0, 0};
    }

    // Helper method to add version info
    void WriteVersion(tinyxml2::XMLElement *element) const
    {
        element->SetAttribute("version", GetVersion().toString().c_str());
    }

    Version ReadVersion(const tinyxml2::XMLElement *element) const
    {
        const char *versionStr = element->Attribute("version");
        if (versionStr)
        {
            return Version::fromString(versionStr);
        }
        return {1, 0, 0}; // Default version
    }
};

// Serializer utilities
namespace SerializerUtils
{
template <typename T>
inline void WriteAttribute(tinyxml2::XMLElement *element, const char *name, const T &value)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        element->SetAttribute(name, value.c_str());
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        element->SetAttribute(name, value ? "true" : "false");
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        element->SetAttribute(name, static_cast<unsigned int>(value));
    }
    else if constexpr (std::is_integral_v<T>)
    {
        element->SetAttribute(name, static_cast<int>(value));
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        element->SetAttribute(name, value);
    }
}

template <typename T>
inline bool ReadAttribute(const tinyxml2::XMLElement *element, const char *name, T &value)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        const char *str = element->Attribute(name);
        if (str)
        {
            value = str;
            return true;
        }
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        const char *str = element->Attribute(name);
        if (str)
        {
            value = (strcmp(str, "true") == 0);
            return true;
        }
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        unsigned int temp;
        bool success = element->QueryUnsignedAttribute(name, &temp) == tinyxml2::XML_SUCCESS;
        if (success)
        {
            value = static_cast<T>(temp);
        }
        return success;
    }
    else if constexpr (std::is_integral_v<T>)
    {
        int temp;
        bool success = element->QueryIntAttribute(name, &temp) == tinyxml2::XML_SUCCESS;
        if (success)
        {
            value = static_cast<T>(temp);
        }
        return success;
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        return element->QueryFloatAttribute(name, &value) == tinyxml2::XML_SUCCESS;
    }
    return false;
}

template <typename T>
inline void WriteElement(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parent, const char *name,
                         const T &value)
{
    auto element = doc.NewElement(name);
    if constexpr (std::is_same_v<T, std::string>)
    {
        element->SetText(value.c_str());
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        element->SetText(static_cast<unsigned int>(value));
    }
    else
    {
        element->SetText(value);
    }
    parent->LinkEndChild(element);
}

template <typename T>
inline bool ReadElement(const tinyxml2::XMLElement *parent, const char *name, T &value)
{
    auto element = parent->FirstChildElement(name);
    if (!element)
        return false;

    if constexpr (std::is_same_v<T, std::string>)
    {
        const char *text = element->GetText();
        if (!text)
            return false;
        value = text;
        return true;
    }
    else if constexpr (std::is_unsigned_v<T>)
    {
        unsigned int temp;
        bool success = element->QueryUnsignedText(&temp) == tinyxml2::XML_SUCCESS;
        if (success)
        {
            value = static_cast<T>(temp);
        }
        return success;
    }
    else if constexpr (std::is_integral_v<T>)
    {
        int temp;
        bool success = element->QueryIntText(&temp) == tinyxml2::XML_SUCCESS;
        if (success)
        {
            value = static_cast<T>(temp);
        }
        return success;
    }
    else if constexpr (std::is_same_v<T, bool>)
    {
        return element->QueryBoolText(&value) == tinyxml2::XML_SUCCESS;
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        return element->QueryFloatText(&value) == tinyxml2::XML_SUCCESS;
    }
    return false;
}

// Helper for serializing Vec3
inline void WriteVec3(tinyxml2::XMLDocument &doc, tinyxml2::XMLElement *parent, const char *name,
                      const glm::vec3 &vec)
{
    auto element = doc.NewElement(name);
    WriteAttribute(element, "x", vec.x);
    WriteAttribute(element, "y", vec.y);
    WriteAttribute(element, "z", vec.z);
    parent->LinkEndChild(element);
}

inline bool ReadVec3(const tinyxml2::XMLElement *parent, const char *name, glm::vec3 &vec,
                     const glm::vec3 &defaultVal = glm::vec3(0, 0, 0))
{
    auto element = parent->FirstChildElement(name);
    if (!element)
    {
        vec = defaultVal;
        return false; // Element not found, but we set default
    }

    float x = defaultVal.x, y = defaultVal.y, z = defaultVal.z;
    ReadAttribute(element, "x", x);
    ReadAttribute(element, "y", y);
    ReadAttribute(element, "z", z);
    vec = glm::vec3(x, y, z);
    return true;
}
} // namespace SerializerUtils

#define SERIALIZE_PROPERTY(element, context, property)                                             \
    SerializerUtils::WriteAttribute(element, #property, property)

#define DESERIALIZE_PROPERTY(element, context, property)                                           \
    SerializerUtils::ReadAttribute(element, #property, property)

#define SERIALIZE_VEC3(context, parent, name, vec)                                                 \
    SerializerUtils::WriteVec3(context.document, parent, name, vec)

#define DESERIALIZE_VEC3(parent, name, vec, defaultVal)                                            \
    SerializerUtils::ReadVec3(parent, name, vec, defaultVal)