/**
 * @file ScriptContext.h
 * @brief Script execution context for Python scripts
 * 
 * This file defines the ScriptContext class which manages the execution
 * environment for individual Python scripts. It handles script loading,
 * execution, and interaction with the C++ simulation environment.
 */

#pragma once
#include <pybind11/pybind11.h>
#include <memory>
#include <string>
#include "../Components/Entity.h"

namespace py = pybind11;
using namespace drosim::game_entity;

namespace drosim::scripting {

    /**
     * @class ScriptContext
     * @brief Manages the execution context for a single Python script
     * 
     * This class is responsible for:
     * - Loading and initializing Python scripts
     * - Managing script lifecycle
     * - Providing script access to entity components
     * - Handling script attributes and method calls
     */
    class ScriptContext {
    public:
        /**
         * @brief Constructs a new script context
         * @param scriptPath Path to the Python script file
         * @param owner Entity that owns this script
         */
        ScriptContext(const std::string& scriptPath, entity owner);
        
        /**
         * @brief Destructor that ensures proper cleanup of Python resources
         */
        ~ScriptContext();
        
        /**
         * @brief Initializes the script context
         * 
         * This method is responsible for loading and initializing the Python
         * script. It must be called before the script can be executed.
         * 
         * @return True if initialization was successful, false otherwise
         */
        bool Initialize();
        
        /**
         * @brief Updates the script context
         * 
         * This method is called repeatedly to update the script's state. It
         * is responsible for executing the script's update logic.
         * 
         * @param deltaTime Time elapsed since the last update
         */
        void Update(float deltaTime);
        
        /**
         * @brief Reloads the script context
         * 
         * This method reloads the Python script and reinitializes the script
         * context. It is typically called when the script has been modified.
         */
        void Reload();
        
        /**
         * @brief Sets a Python attribute in the script's namespace
         * @param name Name of the attribute to set
         * @param value Value to set the attribute to
         * @tparam T Type of the value being set
         * 
         * This template method allows setting any C++ value as a Python
         * attribute in the script's namespace. The value must be of a type
         * that pybind11 can convert to Python.
         */
        template<typename T>
        void SetAttribute(const std::string& name, T value) {
            if (moduleNamespace) {
                moduleNamespace[name.c_str()] = py::cast(value);
            }
        }

    private:
        std::string scriptPath;      ///< Path to the Python script file
        entity ownerEntity;          ///< Entity that owns this script
        py::module_ scriptModule;    ///< Python module for the script
        py::dict moduleNamespace;    ///< Python namespace for the script
        py::object scriptInstance;   ///< Python instance of the script
        
        /**
         * @brief Sets up the Python environment for the script
         * 
         * This method is responsible for setting up the Python environment
         * for the script, including importing necessary modules and setting
         * up the script's namespace.
         */
        void SetupEnvironment();
    };

} // namespace drosim::scripting