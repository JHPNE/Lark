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
    content_tools::scene *scene;
    quad_params params;
    drone_state drone_state;
    control_abstraction control_abstraction;
    control_input input;
    trajectory trajectory;
};

struct game_entity_descriptor {
    transform_component transform;
    script_component script;
    geometry_component geometry;
    physics_component physics;
};