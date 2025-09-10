#pragma once
#include <CommonHeaders.h>
#include "LarkAPI/GameEntity.h"
#include "GeometryStructures.h"
#include "PhysicsStructures.h"

// Component Structures

struct transform_component {
    float position[3];
    float rotation[3];
    float scale[3];
};

struct script_component {
    lark::script::detail::script_creator script_creator; // Use the actual type instead of void*
};

enum GeometryType {
    PrimitiveType,
    ObjImport,
};

struct geometry_component {
    content_tools::scene *scene;
    bool is_dynamic = false;
    const char *name;
    const char *file_name;
    GeometryType type;
    content_tools::PrimitiveMeshType mesh_type;
};

struct physics_component {
    physics_inertia_properties inertia;
    physics_aerodynamic_properties aerodynamic;
    physics_motor_properties motor;
    std::vector<physics_rotor_parameters> rotors;
    PhysicsControlMode control_mode = PhysicsControlMode::MOTOR_SPEEDS;
    physics_wind_params wind_params;
};

struct game_entity_descriptor {
    transform_component transform;
    script_component script;
    geometry_component geometry;
    physics_component physics;
};