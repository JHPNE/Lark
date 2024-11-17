#include "PythonBindings.h"
#include <pybind11/operators.h>
#include <pybind11/stl.h>

namespace drosim::scripting {

    void PythonBindings::RegisterTypes(pybind11::module_& m) {
        // Register basic types
        py::class_<entity>(m, "Entity")
            .def("is_valid", &entity::is_valid);

        /*
        // Register Vec3 with operator overloads
        py::class_<Vec3>(m, "Vec3")
            .def(py::init<float, float, float>())
            .def("__add__", &Vec3::operator+)
            .def("__sub__", &Vec3::operator-)
            .def_readwrite("x", &Vec3::x)
            .def_readwrite("y", &Vec3::y)
            .def_readwrite("z", &Vec3::z);
        */
        RegisterComponents(m);
        RegisterSystems(m);
        RegisterMath(m);
    }

    void PythonBindings::RegisterComponents(pybind11::module_& m) {
        // Register component types
        auto components = m.def_submodule("components");
        /*
        py::class_<Transform>(components, "Transform")
            .def_property("position", &Transform::GetPosition, &Transform::SetPosition)
            .def_property("rotation", &Transform::GetRotation, &Transform::SetRotation)
            .def_property("scale", &Transform::GetScale, &Transform::SetScale);
         */
    }

} // namespace DroSim::Scripting