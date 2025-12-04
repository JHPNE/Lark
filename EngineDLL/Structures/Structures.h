#pragma once
#include <CommonHeaders.h>
#include "LarkAPI/GameEntity.h"
#include "GeometryStructures.h"
#include "DroneStructures.h"

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
    float mass;
    glm::vec3 position;
    glm::vec4 orientation;
    glm::vec3 inertia;
    content_tools::scene *scene;
    bool is_kinematic;
};

struct drone_component {
    quad_params params;
    control_abstraction control_abstraction;
    trajectory trajectory;
    drone_state drone_state;
    control_input input;
};

// Empty for now
struct material_component {
};

struct game_entity_descriptor {
    transform_component transform;
    script_component script;
    geometry_component geometry;
    physics_component physics;
    drone_component drone;
    material_component material;
};