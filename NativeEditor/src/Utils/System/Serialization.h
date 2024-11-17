#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>
#include <functional>
#include "tinyxml2.h"

class SerializationContext {
public:
    explicit SerializationContext(tinyxml2::XMLDocument& doc) : document(doc) {}
    tinyxml2::XMLDocument& document;
};

class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void Serialize(tinyxml2::XMLElement* element, SerializationContext& context) const = 0;
    virtual bool Deserialize(const tinyxml2::XMLElement* element, SerializationContext& context) = 0;
};

// Serializer utilities
namespace SerializerUtils {
    template<typename T>
    inline void WriteAttribute(tinyxml2::XMLElement* element, const char* name, const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            element->SetAttribute(name, value.c_str());
        }
        else if constexpr (std::is_unsigned_v<T>) {
            element->SetAttribute(name, static_cast<unsigned int>(value));
        }
        else {
            element->SetAttribute(name, value);
        }
    }

    template<typename T>
    inline bool ReadAttribute(const tinyxml2::XMLElement* element, const char* name, T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            const char* str = element->Attribute(name);
            if (str) {
                value = str;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return element->QueryBoolAttribute(name, &value) == tinyxml2::XML_SUCCESS;
        }
        else if constexpr (std::is_unsigned_v<T>) {
            unsigned int temp;
            bool success = element->QueryUnsignedAttribute(name, &temp) == tinyxml2::XML_SUCCESS;
            if (success) {
                value = static_cast<T>(temp);
            }
            return success;
        }
        else if constexpr (std::is_integral_v<T>) {
            int temp;
            bool success = element->QueryIntAttribute(name, &temp) == tinyxml2::XML_SUCCESS;
            if (success) {
                value = static_cast<T>(temp);
            }
            return success;
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return element->QueryFloatAttribute(name, &value) == tinyxml2::XML_SUCCESS;
        }
        return false;
    }

    template<typename T>
    inline void WriteElement(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent,
                           const char* name, const T& value) {
        auto element = doc.NewElement(name);
        if constexpr (std::is_same_v<T, std::string>) {
            element->SetText(value.c_str());
        }
        else if constexpr (std::is_unsigned_v<T>) {
            element->SetText(static_cast<unsigned int>(value));
        }
        else {
            element->SetText(value);
        }
        parent->LinkEndChild(element);
    }

    template<typename T>
    inline bool ReadElement(const tinyxml2::XMLElement* parent, const char* name, T& value) {
        auto element = parent->FirstChildElement(name);
        if (!element) return false;

        if constexpr (std::is_same_v<T, std::string>) {
            const char* text = element->GetText();
            if (!text) return false;
            value = text;
            return true;
        }
        else if constexpr (std::is_unsigned_v<T>) {
            unsigned int temp;
            bool success = element->QueryUnsignedText(&temp) == tinyxml2::XML_SUCCESS;
            if (success) {
                value = static_cast<T>(temp);
            }
            return success;
        }
        else if constexpr (std::is_integral_v<T>) {
            int temp;
            bool success = element->QueryIntText(&temp) == tinyxml2::XML_SUCCESS;
            if (success) {
                value = static_cast<T>(temp);
            }
            return success;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            return element->QueryBoolText(&value) == tinyxml2::XML_SUCCESS;
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return element->QueryFloatText(&value) == tinyxml2::XML_SUCCESS;
        }
        return false;
    }
}