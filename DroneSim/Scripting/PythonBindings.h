/**
 * @file PythonBindings.h
 * @brief Python binding system for exposing C++ functionality to Python scripts
 * 
 * This file defines the interface for exposing C++ classes, functions, and
 * components to Python scripts. It uses pybind11 to create the bindings and
 * manage the interaction between C++ and Python code.
 */

#pragma once
#include <pybind11/pybind11.h>
#include "../Components/Entity.h"
namespace py = pybind11;
using namespace drosim::game_entity;

namespace drosim::scripting {

    /**
     * @class PythonBindings
     * @brief Manages Python bindings for the simulation
     * 
     * This class is responsible for registering C++ types with Python
     * and managing the interaction between C++ and Python code.
     */
    class PythonBindings {
    public:
        /**
         * @brief Registers all component types with Python
         * @param m Python module to register components with
         * 
         * This method creates Python bindings for all component types
         * (Transform, Script, etc.) making them accessible from Python scripts.
         */
        static void RegisterComponents(pybind11::module_& m);

        /**
         * @brief Registers utility functions with Python
         * @param m Python module to register utilities with
         * 
         * This method creates Python bindings for utility functions
         * that are useful for script development.
         */
        static void RegisterUtilities(pybind11::module_& m);

        static void RegisterTypes(pybind11::module_& m);

    private:
        static void RegisterSystems(pybind11::module_& m);
        static void RegisterMath(pybind11::module_& m);
    };

} // namespace drosim::scripting