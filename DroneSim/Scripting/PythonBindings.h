#pragma once
#include <pybind11/pybind11.h>
#include "../Components/Entity.h"
namespace py = pybind11;
using namespace drosim::game_entity;

namespace drosim::scripting {

    class PythonBindings {
    public:
        static void RegisterTypes(pybind11::module_& m);

    private:
        static void RegisterComponents(pybind11::module_& m);
        static void RegisterSystems(pybind11::module_& m);
        static void RegisterMath(pybind11::module_& m);
    };

} // namespace DroSim::Scripting