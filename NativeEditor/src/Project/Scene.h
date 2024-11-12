// Scene.h
#pragma once
#include <string>
#include <memory>

class Project;

class Scene {
public:
    Scene(const std::string& name, uint32_t id, std::shared_ptr<Project> owner)
        : m_name(name), m_id(id), m_owner(owner) {}

    const std::string& GetName() const { return m_name; }
	uint32_t GetID() const { return m_id; }
    std::shared_ptr<Project> GetOwner() const { return m_owner; }

private:
    std::string m_name;
	uint32_t m_id;
    std::shared_ptr<Project> m_owner;
};