/**
 * @file MeshPrimitives.h
 * @brief Basic geometric primitive generation
 *
 * This file provides functionality for generating basic 3D geometric primitives
 * such as planes, cubes, spheres, etc. These primitives can be used as building
 * blocks for more complex geometry or for testing and debugging purposes.
 */

#pragma once
#include "../Common/CommonHeaders.h"
#include "Geometry.h"

namespace lark::tools
{

/**
 * @enum primitive_mesh_type
 * @brief Type of primitive mesh to generate
 *
 * Enumerates the different types of primitive meshes that can be generated.
 */
enum primitive_mesh_type : u32
{
    plane,      ///< Plane primitive
    cube,       ///< Cube primitive
    uv_sphere,  ///< UV sphere primitive
    ico_sphere, ///< Icosahedron-based sphere primitive
    cylinder,   ///< Cylinder primitive
    capsule,    ///< Capsule primitive

    count ///< Number of primitive mesh types
};

/**
 * @struct primitive_init_info
 * @brief Initialization info for primitive mesh generation
 *
 * Contains parameters that control how primitive meshes are generated,
 * including size, segments, and other shape-specific parameters.
 */
struct primitive_init_info
{
    primitive_mesh_type mesh_type; ///< Type of primitive mesh to generate
    u32 segments[3]{1, 1, 1};      ///< Number of segments for subdivision
    math::v3 size{1, 1, 1};        ///< Base size of the primitive
    u32 lod{0};                    ///< Level of detail
};

/**
 * @brief Creates a primitive mesh
 * @param data Scene data to add the mesh to
 * @param info Initialization info for the primitive mesh
 *
 * Generates a primitive mesh based on the provided initialization info
 * and adds it to the scene data.
 */
void CreatePrimitiveMesh(scene_data *data, primitive_init_info *info);
} // namespace lark::tools