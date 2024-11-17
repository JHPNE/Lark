#pragma once
#include <pybind11/pybind11.h>
#include <memory>
#include <string>
#include "../Components/Entity.h"

namespace py = pybind11;
using namespace drosim::game_entity;

class ScriptContext {
public:
    ScriptContext(const std::string& scriptPath, entity owner);
    ~ScriptContext();
    
    bool Initialize();
    void Update(float deltaTime);
    void Reload();
    
    template<typename T>
    void SetAttribute(const std::string& name, T value) {
        if (moduleNamespace) {
            moduleNamespace[name.c_str()] = py::cast(value);
        }
    }
    
private:
    std::string scriptPath;
    entity ownerEntity;
    py::module_ scriptModule;
    py::dict moduleNamespace;
    py::object scriptInstance;
    
    void SetupEnvironment();
};