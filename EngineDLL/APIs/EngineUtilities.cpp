#include "EngineUtilities.h"
#include "Core/GameLoop.h"
#include "Geometry.h"
#include "Geometry/MeshPrimitives.h"
#include "Physics.h"
#include "Script.h"
#include "Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace lark;

namespace engine
{

namespace {
   Eigen::Vector3f glmToEigenV3(glm::vec3 vec)
   {
       return Eigen::Vector3f{ vec.x, vec.y, vec.z };
   }

   Eigen::Vector4f glmToEigenV4(glm::vec4 vec)
   {
       return Eigen::Vector4f{ vec.x, vec.y, vec.z, vec.w};
   }


   drones::ControlAbstraction extractAbstraction(control_abstraction abs)
   {
       switch (abs)
       {
           case control_abstraction::CMD_ACC:
               return drones::ControlAbstraction::CMD_ACC;
           case control_abstraction::CMD_VEL:
               return drones::ControlAbstraction::CMD_VEL;
           case control_abstraction::CMD_CTATT:
               return drones::ControlAbstraction::CMD_CTATT;
           case control_abstraction::CMD_CTBM:
               return drones::ControlAbstraction::CMD_CTBM;
           case control_abstraction::CMD_CTBR:
               return drones::ControlAbstraction::CMD_CTBR;
           case control_abstraction::CMD_MOTOR_THRUSTS:
               return drones::ControlAbstraction::CMD_MOTOR_THRUSTS;
           case control_abstraction::CMD_MOTOR_SPEEDS:
               return drones::ControlAbstraction::CMD_MOTOR_SPEEDS;
       }
   }
    drones::QuadParams extractParams(quad_params params) {
       drones::QuadParams result;
        // inertia
        result.inertia_properties.mass = params.i.mass;
        result.inertia_properties.principal_inertia = glmToEigenV3(params.i.principal_inertia);
        result.inertia_properties.product_inertia = glmToEigenV3(params.i.product_inertia);

        // geom
        //info.params.geometric_properties.num_rotors = physics.params.g.num_rotors;
        result.geometric_properties.rotor_radius = params.g.rotor_radius;

        for (int i = 0; i < params.g.num_rotors; ++i)
        {
            result.geometric_properties.rotor_positions[i] = glmToEigenV3(params.g.rotor_positions[i]);
        }
        result.geometric_properties.rotor_directions = glmToEigenV4(params.g.rotor_directions);
        result.geometric_properties.imu_position = glmToEigenV3(params.g.imu_positions);

        // aero
        result.aero_dynamics_properties.parasitic_drag = glmToEigenV3(params.a.parasitic_drag);

        // rotor
        result.rotor_properties.k_eta = params.r.k_eta;
        result.rotor_properties.k_m = params.r.k_m;
        result.rotor_properties.k_d = params.r.k_d;
        result.rotor_properties.k_z = params.r.k_z;
        result.rotor_properties.k_h = params.r.k_h;
        result.rotor_properties.k_flap = params.r.k_flap;

        // motor
        result.motor_properties.tau_m = params.m.tau_m;
        result.motor_properties.rotor_speed_min = params.m.rotor_speed_min;
        result.motor_properties.rotor_speed_max = params.m.rotor_speed_max;
        result.motor_properties.motor_noise_std = params.m.motor_noise_std;

        // Control gains
        result.control_gains.kp_pos = glmToEigenV3(params.c.kp_pos);
        result.control_gains.kd_pos = glmToEigenV3(params.c.kd_pos);
        result.control_gains.kp_att = params.c.kp_att;
        result.control_gains.kd_att = params.c.kd_att;
        result.control_gains.kp_vel = glmToEigenV3(params.c.kp_vel);

        result.lower_level_controller_properties.k_w = params.l.k_w;
        result.lower_level_controller_properties.k_v = params.l.k_v;
        result.lower_level_controller_properties.kp_att = params.l.kp_att;
        result.lower_level_controller_properties.kd_att = params.l.kd_att;

       return result;
   }

   drones::ControlInput extractControlInput(control_input input)
   {
       drones::ControlInput result;
       result.cmd_acc = glmToEigenV3(input.cmd_acc);
       result.cmd_v = glmToEigenV3(input.cmd_v);
       result.cmd_w = glmToEigenV3(input.cmd_w);

       result.cmd_q = glmToEigenV4(input.cmd_q);
       result.cmd_moment = glmToEigenV3(input.cmd_moment);
       result.cmd_thrust = input.cmd_thrust;

       result.cmd_motor_thrusts = glmToEigenV4(input.cmd_motor_thrusts);
       result.cmd_motor_speeds = glmToEigenV4(input.cmd_motor_speeds);

       return result;
   }

   drones::DroneState extractState(drone_state state)
   {
       drones::DroneState result{};
       result.position = glmToEigenV3(state.position);
       result.velocity = glmToEigenV3(state.velocity);
       result.attitude = glmToEigenV4(state.attitude);
       result.body_rates = glmToEigenV3(state.body_rates);
       result.wind = glmToEigenV3(state.wind);
       result.rotor_speeds = glmToEigenV4(state.rotor_speeds);

       return result;
   }

   util::vector<lark::tools::lod_group> extractLodGroups(content_tools::scene *scene)
   {
       util::vector<lark::tools::lod_group> lod_groups;
       for (auto comp_lod_group : scene->lod_groups)
       {
           lark::tools::lod_group lod_group;
           lod_group.name = comp_lod_group.name;

           util::vector<lark::tools::mesh> meshes;
           for (auto mesh : comp_lod_group.meshes)
           {
               lark::tools::mesh m;
               m.name = mesh.name;
               m.positions = mesh.positions;
               m.normals = mesh.normals;
               m.tangents = mesh.tangents;
               m.uv_sets = mesh.uv_sets;
               m.raw_indices = mesh.raw_indices;

               util::vector<lark::tools::vertex> vertices;
               for (auto vertex : mesh.vertices)
               {
                   lark::tools::vertex v;
                   v.tangent = vertex.tangent;
                   v.position = vertex.position;
                   v.normal = vertex.normal;
                   v.uv = vertex.uv;

                   vertices.emplace_back(v);
               }
               m.vertices = vertices;
               m.indices = mesh.indices;
               m.lod_threshold = mesh.lod_threshold;
               m.lod_id = mesh.lod_id;
               m.is_dynamic = false;

               util::vector<lark::tools::packed_vertex::vertex_static> packed_vertices_static;
               m.packed_vertices_static = packed_vertices_static;

               meshes.emplace_back(m);
           }
           lod_group.meshes = meshes;
           lod_groups.emplace_back(lod_group);
       }

       return lod_groups;
   }


   std::shared_ptr<drones::Trajectory> extractTrajectory(trajectory trajectory)
   {
       switch (trajectory.type)
       {
           case trajectory_type::Circular:
               return std::make_shared<drones::Circular>(
              glmToEigenV3(trajectory.position),
                    trajectory.radius,  // radius
                    trajectory.frequency,  // frequency
                    true   // yaw_bool
                );

           case trajectory_type::Chaos:
               return std::make_shared<drones::Chaos>(
             glmToEigenV3(trajectory.position),
                   trajectory.delta,   // delta
                   trajectory.n_points,     // n_points
                   trajectory.segment_time
               );
       }
   }

}

std::unique_ptr<GameLoop> g_game_loop;
util::vector<bool> active_entities;

// Internal implementation of transform conversion
transform::init_info to_engine_transform(const transform_component &transform)
{
    transform::init_info info{};
    memcpy(&info.position[0], &transform.position[0], sizeof(float) * 3);
    memcpy(&info.scale[0], &transform.scale[0], sizeof(float) * 3);

    glm::vec3 euler_angles(transform.rotation[0], transform.rotation[1], transform.rotation[2]);
    glm::quat quaternion = glm::quat(euler_angles);

    info.rotation[0] = quaternion.x;
    info.rotation[1] = quaternion.y;
    info.rotation[2] = quaternion.z;
    info.rotation[3] = quaternion.w;

    return info;
}

// Internal implementation of script conversion
script::init_info to_engine_script(const script_component &script)
{
    script::init_info info{};
    info.script_creator = script.script_creator; // Direct assignment since types match
    return info;
}

geometry::init_info to_engine_geometry(const geometry_component &geometry)
{
    geometry::init_info info{};

    if (geometry.scene == nullptr || geometry.scene->lod_groups.empty())
        return info;

    info.is_dynamic = false;
    info.scene = std::make_shared<lark::tools::scene>();
    info.scene->lod_groups = extractLodGroups(geometry.scene);

    if (!geometry.scene->name.empty())
    {
        info.scene->name = geometry.scene->name;
    }

    return info;
}

physics::init_info to_engine_physics(const physics_component &physics)
{
    physics::init_info info{};
    if (physics.scene == nullptr || physics.params.i.mass == 0.0f)
        return info;

    info.params = extractParams(physics.params);

    info.abstraction = extractAbstraction(physics.control_abstraction);

    info.last_control = extractControlInput(physics.input);

    info.scene = std::make_shared<lark::tools::scene>();
    info.scene->lod_groups = extractLodGroups(physics.scene);
    info.scene->name = physics.scene->name;

    info.state = extractState(physics.drone_state);

    info.trajectory = extractTrajectory(physics.trajectory);

    return info;
}

game_entity::entity entity_from_id(id::id_type id)
{
    return game_entity::entity{game_entity::entity_id(id)};
}

bool is_entity_valid(id::id_type id)
{
    if (!id::is_valid(id))
        return false;
    const id::id_type index{id::index(id)};
    if (index >= active_entities.size())
        return false;
    return active_entities[index];
}

void remove_entity(id::id_type id)
{
    if (!is_entity_valid(id))
        return;

    game_entity::entity_id entity_id{id};
    if (game_entity::is_alive(entity_id))
    {
        auto entity = game_entity::entity{entity_id};
        if (auto script_comp = entity.script(); script_comp.is_valid())
        {
            script::remove(script_comp);
        }
        game_entity::remove(entity_id);
    }

    const auto index = id::index(id);
    active_entities[index] = false;
}

// Convert from ContentTools types to Engine types
lark::tools::primitive_mesh_type ConvertPrimitiveType(content_tools::PrimitiveMeshType type)
{
    return static_cast<lark::tools::primitive_mesh_type>(type);
}

std::shared_ptr<drones::Wind> chooseWind(const wind& wind_config)
{
    switch (wind_config.type)
    {
    case wind_type::ConstantWind:
        return std::make_shared<drones::ConstantWind>(
            Eigen::Vector3f(wind_config.w.x, wind_config.w.y, wind_config.w.z)
        );

    case wind_type::SinusoidWind:
        return std::make_shared<drones::SinusoidWind>(
            Eigen::Vector3f(wind_config.amplitudes.x, wind_config.amplitudes.y, wind_config.amplitudes.z),
            Eigen::Vector3f(wind_config.frequencies.x, wind_config.frequencies.y, wind_config.frequencies.z),
            Eigen::Vector3f(wind_config.phase.x, wind_config.phase.y, wind_config.phase.z)
        );

    case wind_type::LadderWind:
        return std::make_shared<drones::LadderWind>(
            Eigen::Vector3f(wind_config.min.x, wind_config.min.y, wind_config.min.z),
            Eigen::Vector3f(wind_config.max.x, wind_config.max.y, wind_config.max.z),
            Eigen::Vector3f(wind_config.duration.x, wind_config.duration.y, wind_config.duration.z),
            Eigen::Vector3f(wind_config.n_steps.x, wind_config.n_steps.y, wind_config.n_steps.z),
            wind_config.random
        );

    case wind_type::NoWind:
    default:
        return std::make_shared<drones::NoWind>();
    }
}

void cleanup_engine_systems()
{
    std::vector<id::id_type> to_remove;
    for (id::id_type i = 0; i < active_entities.size(); ++i)
    {
        if (active_entities[i])
        {
            to_remove.push_back(i);
        }
    }

    for (auto id : to_remove)
    {
        remove_entity(id);
    }

    active_entities.clear();
    script::shutdown();
}
} // namespace engine