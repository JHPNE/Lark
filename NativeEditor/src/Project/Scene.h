// Scene.h
#pragma once
#include <string>
#include <memory>

class Project;

class Scene {
public:
    Scene(const std::string& name, std::shared_ptr<Project> owner)
        : m_name(name), m_owner(owner) {}

    const std::string& GetName() const { return m_name; }
    std::shared_ptr<Project> GetOwner() const { return m_owner; }

private:
    std::string m_name;
    std::shared_ptr<Project> m_owner;
};