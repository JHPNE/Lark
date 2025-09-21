#pragma once
#include "Component.h"
#include "EngineAPI.h"
#include "Utils/System/Serialization.h"
#include <string>

using namespace MathUtils;

class Drone : public Component, public ISerializable
{
    public:
        explicit Drone(GameEntity *owner) : Component(owner) {}

        [[nodiscard]] ComponentType GetType() const override { return GetStaticType(); }
        static ComponentType GetStaticType() { return ComponentType::Drone; }

        bool Initialize(const ComponentInitializer *init) override
        {
            if (init)
            {
                const auto *DroneInit = static_cast<const DroneInitializer*>(init);
                m_params = DroneInit->params;
                m_control_abstraction = DroneInit->control_abstraction;
                m_trajectory = DroneInit->trajectory;
                m_drone_state = DroneInit->drone_state;
                m_input = DroneInit->input;
            }

            return true;
        };

        void Serialize(tinyxml2::XMLElement *element, SerializationContext &context) const override
        {
            WriteVersion(element);

            auto paramsElement = context.document.NewElement("QuadParams");

            // Inertia Property
            auto inertiaElement = context.document.NewElement("InertiaProperty");
            SerializerUtils::WriteAttribute(inertiaElement, "Mass", m_params.i.mass);
            SERIALIZE_VEC3(context, inertiaElement, "PrincipalInertia", m_params.i.principal_inertia);
            SERIALIZE_VEC3(context, inertiaElement, "ProductInertia", m_params.i.product_inertia);
            paramsElement->LinkEndChild(inertiaElement);

            // Geom Property
            auto geometryElement = context.document.NewElement("GeometryProperty");
            SerializerUtils::WriteAttribute(geometryElement, "RotorRadius", m_params.g.rotor_radius);

            auto rotorPosElement = context.document.NewElement("RotorPositions");
            for (int i = 0; i < geom_prop::num_rotors; ++i)
            {
                auto rotorElement = context.document.NewElement(("Rotor_" + std::to_string(i)).c_str());
                SERIALIZE_VEC3(context, rotorElement, "Position", m_params.g.rotor_positions[i]);
                rotorPosElement->LinkEndChild(rotorElement);
            }
            geometryElement->LinkEndChild(rotorPosElement);

            SERIALIZE_VEC4(context, geometryElement, "RotorDirections", m_params.g.rotor_directions);

            paramsElement->LinkEndChild(geometryElement);

            // Aero Prop
            auto aeroElement = context.document.NewElement("AeroProperty");
            SERIALIZE_VEC3(context, aeroElement, "ParasiticDrag", m_params.a.parasitic_drag);
            paramsElement->LinkEndChild(aeroElement);

            // Rotor Prop
            auto rotorPropertyElements = context.document.NewElement("RotorProperties");
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_eta);
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_m);
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_d);
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_z);
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_h);
            SERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_flap);
            paramsElement->LinkEndChild(rotorPropertyElements);

            // Motor Property
            auto motorPropElement = context.document.NewElement("MotorProperty");
            SerializerUtils::WriteAttribute(motorPropElement, "tau_m", m_params.m.tau_m);
            SerializerUtils::WriteAttribute(motorPropElement, "rotor_speed_min", m_params.m.rotor_speed_min);
            SerializerUtils::WriteAttribute(motorPropElement, "rotor_speed_max", m_params.m.rotor_speed_max);
            SerializerUtils::WriteAttribute(motorPropElement, "motor_noise_std", m_params.m.motor_noise_std);
            paramsElement->LinkEndChild(motorPropElement);

            // Control Gains
            auto controlGainsElement = context.document.NewElement("ControlGains");
            SERIALIZE_VEC3(context, controlGainsElement, "kp_pos", m_params.c.kp_pos);
            SERIALIZE_VEC3(context, controlGainsElement, "kd_pos", m_params.c.kd_pos);
            SerializerUtils::WriteAttribute(controlGainsElement, "kp_att", m_params.c.kp_att);
            SerializerUtils::WriteAttribute(controlGainsElement, "kd_att", m_params.c.kd_att);
            SERIALIZE_VEC3(context, controlGainsElement, "kp_vel", m_params.c.kp_vel);
            paramsElement->LinkEndChild(controlGainsElement);


            // Lower Level Controller Property
            auto lowerLevelElement = context.document.NewElement("LowerLevelController");
            SerializerUtils::WriteAttribute(lowerLevelElement, "k_w", m_params.l.k_w);
            SerializerUtils::WriteAttribute(lowerLevelElement, "k_v", m_params.l.k_v);
            SerializerUtils::WriteAttribute(lowerLevelElement, "kp_att", m_params.l.kp_att);
            SerializerUtils::WriteAttribute(lowerLevelElement, "kd_att", m_params.l.kd_att);
            paramsElement->LinkEndChild(lowerLevelElement);

            element->LinkEndChild(paramsElement);

            // Control Abstraction
            auto controlAbstractionElement = context.document.NewElement("ControlAbstraction");
            SerializerUtils::WriteAttribute(controlAbstractionElement, "type", static_cast<int>(m_control_abstraction));
            element->LinkEndChild(controlAbstractionElement);

            // Trajectory
            auto trajectoryElement = context.document.NewElement("Trajectory");
            SerializerUtils::WriteAttribute(trajectoryElement, "type", static_cast<int>(m_trajectory.type));
            SERIALIZE_VEC3(context, trajectoryElement, "position", m_trajectory.position);
            SerializerUtils::WriteAttribute(trajectoryElement, "delta", m_trajectory.delta);
            SerializerUtils::WriteAttribute(trajectoryElement, "radius", m_trajectory.radius);
            SerializerUtils::WriteAttribute(trajectoryElement, "frequency", m_trajectory.frequency);
            SerializerUtils::WriteAttribute(trajectoryElement, "n_points", m_trajectory.n_points);
            SerializerUtils::WriteAttribute(trajectoryElement, "segment_time", m_trajectory.segment_time);
            element->LinkEndChild(trajectoryElement);

            // Drone State
            auto droneStateElement = context.document.NewElement("DroneState");
            SERIALIZE_VEC3(context, droneStateElement, "position", m_drone_state.position);
            SERIALIZE_VEC3(context, droneStateElement, "velocity", m_drone_state.velocity);
            SERIALIZE_VEC4(context, droneStateElement, "attitude", m_drone_state.attitude);
            SERIALIZE_VEC3(context, droneStateElement, "body_rates", m_drone_state.body_rates);
            SERIALIZE_VEC3(context, droneStateElement, "wind", m_drone_state.wind);
            SERIALIZE_VEC4(context, droneStateElement, "rotor_speeds", m_drone_state.rotor_speeds);

            element->LinkEndChild(droneStateElement);

            // Control Input
            auto controlInputElement = context.document.NewElement("ControlInput");
            SERIALIZE_VEC4(context, controlInputElement, "cmd_motor_speeds", m_input.cmd_motor_speeds);
            SERIALIZE_VEC4(context, controlInputElement, "cmd_motor_thrusts", m_input.cmd_motor_thrusts);
            SERIALIZE_PROPERTY(controlInputElement, context, m_input.cmd_thrust);
            SERIALIZE_VEC3(context, controlInputElement, "cmd_moment", m_input.cmd_moment);
            SERIALIZE_VEC4(context, controlInputElement, "cmd_q", m_input.cmd_q);
            SERIALIZE_VEC3(context, controlInputElement, "cmd_w", m_input.cmd_w);
            SERIALIZE_VEC3(context, controlInputElement, "cmd_v", m_input.cmd_v);
            SERIALIZE_VEC3(context, controlInputElement, "cmd_acc", m_input.cmd_acc);

            element->LinkEndChild(controlInputElement);
        };

    bool Deserialize(const tinyxml2::XMLElement *element, SerializationContext &context) override
    {
        context.version = ReadVersion(element);
        if (!SupportsVersion(context.version))
        {
            context.AddError("Unsupported Drone version: " + context.version.toString());
            return false;
        }

        // Deserialize quad_params
        if (auto paramsElement = element->FirstChildElement("QuadParams")) {
            // Inertia Property
            if (auto inertiaElement = paramsElement->FirstChildElement("InertiaProperty"))
            {
                SerializerUtils::ReadAttribute(inertiaElement, "Mass", m_params.i.mass);
                DESERIALIZE_VEC3(inertiaElement, "PrincipalInertia", m_params.i.principal_inertia, glm::vec3(1.0f));
                DESERIALIZE_VEC3(inertiaElement, "ProductInertia", m_params.i.product_inertia, glm::vec3(0.0f));
            }

            // Geometry Property
            if (auto geometryElement = paramsElement->FirstChildElement("GeometryProperty")) {
                SerializerUtils::ReadAttribute(geometryElement, "RotorRadius", m_params.g.rotor_radius);

                if (auto rotorPosElement = geometryElement->FirstChildElement("RotorPositions"))
                {
                    for (int i = 0; i < geom_prop::num_rotors; ++i)
                    {
                        auto rotorElement = rotorPosElement->FirstChildElement(("Rotor_" + std::to_string(i)).c_str());
                        if (rotorElement)
                        {
                            DESERIALIZE_VEC3(rotorElement, "Position", m_params.g.rotor_positions[i], glm::vec3(0.0f));
                        }
                    }
                }
                DESERIALIZE_VEC4(geometryElement, "RotorDirections", m_params.g.rotor_directions, glm::vec4(0.f));
            }

            // Aero Property
            if (auto aeroElement = paramsElement->FirstChildElement("AeroProperty"))
            {
                DESERIALIZE_VEC3(aeroElement, "ParasiticDrag", m_params.a.parasitic_drag, glm::vec3(0.0f));
            }

            // Rotor Property
            if (auto rotorPropertyElements = paramsElement->FirstChildElement("RotorProperty"))
            {
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_eta);
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_m);
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_d);
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_z);
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_h);
                DESERIALIZE_PROPERTY(rotorPropertyElements, context, m_params.r.k_flap);
            }

            // Motor Property
            if (auto motorPropElement = paramsElement->FirstChildElement("MotorProperty"))
            {
                SerializerUtils::ReadAttribute(motorPropElement, "tau_m", m_params.m.tau_m);
                SerializerUtils::ReadAttribute(motorPropElement, "rotor_speed_min", m_params.m.rotor_speed_min);
                SerializerUtils::ReadAttribute(motorPropElement, "rotor_speed_max", m_params.m.rotor_speed_max);
                SerializerUtils::ReadAttribute(motorPropElement, "motor_noise_std", m_params.m.motor_noise_std);
            }

            // Control Gains
            if (auto controlGainsElement = paramsElement->FirstChildElement("ControlGains"))
            {
                DESERIALIZE_VEC3(controlGainsElement, "kp_pos", m_params.c.kp_pos, glm::vec3(6.5f, 6.5f, 15.0f));
                DESERIALIZE_VEC3(controlGainsElement, "kd_pos", m_params.c.kd_pos, glm::vec3(4.0f, 4.0f, 9.0f));
                SerializerUtils::ReadAttribute(controlGainsElement, "kp_att", m_params.c.kp_att);
                SerializerUtils::ReadAttribute(controlGainsElement, "kd_att", m_params.c.kd_att);
                DESERIALIZE_VEC3(controlGainsElement, "kp_vel", m_params.c.kp_vel, glm::vec3(0.65f, 0.65f, 1.5f));
            }

            // Lower Level Controller Property
            if (auto lowerLevelElement = paramsElement->FirstChildElement("LowerLevelController"))
            {
                SerializerUtils::ReadAttribute(lowerLevelElement, "k_w", m_params.l.k_w);
                SerializerUtils::ReadAttribute(lowerLevelElement, "k_v", m_params.l.k_v);
                SerializerUtils::ReadAttribute(lowerLevelElement, "kp_att", m_params.l.kp_att);
                SerializerUtils::ReadAttribute(lowerLevelElement, "kd_att", m_params.l.kd_att);
            }
        }

        // Control Abstraction
        if (auto controlAbstractionElement = element->FirstChildElement("ControlAbstraction"))
        {
            int type = 0;
            SerializerUtils::ReadAttribute(controlAbstractionElement, "type", type);
            m_control_abstraction = static_cast<control_abstraction>(type);
        }

        // Trajectory
        if (auto trajectoryElement = element->FirstChildElement("Trajectory"))
        {
            int type = 0;
            SerializerUtils::ReadAttribute(trajectoryElement, "type", type);
            m_trajectory.type = static_cast<trajectory_type>(type);
            DESERIALIZE_VEC3(trajectoryElement, "position", m_trajectory.position, glm::vec3(0.0f));
            SerializerUtils::ReadAttribute(trajectoryElement, "delta", m_trajectory.delta);
            SerializerUtils::ReadAttribute(trajectoryElement, "radius", m_trajectory.radius);
            SerializerUtils::ReadAttribute(trajectoryElement, "frequency", m_trajectory.frequency);
            SerializerUtils::ReadAttribute(trajectoryElement, "n_points", m_trajectory.n_points);
            SerializerUtils::ReadAttribute(trajectoryElement, "segment_time", m_trajectory.segment_time);
        }

        // Drone State
        if (auto droneStateElement = element->FirstChildElement("DroneState")) {
            DESERIALIZE_VEC3(droneStateElement, "position", m_drone_state.position, glm::vec3(0.0f));
            DESERIALIZE_VEC3(droneStateElement, "velocity", m_drone_state.velocity, glm::vec3(0.f));
            DESERIALIZE_VEC4(droneStateElement, "attitude", m_drone_state.attitude, glm::vec4(0.f));
            DESERIALIZE_VEC3(droneStateElement, "body_rates", m_drone_state.body_rates, glm::vec3(0.f));
            DESERIALIZE_VEC3(droneStateElement, "wind", m_drone_state.wind, glm::vec3(0.f));
            DESERIALIZE_VEC4(droneStateElement, "rotor_speeds", m_drone_state.rotor_speeds, glm::vec4(0.f));
        }

        if (auto controlInputElement = element->FirstChildElement("ControlInput")) {
            DESERIALIZE_VEC4(controlInputElement, "cmd_motor_speeds", m_input.cmd_motor_speeds, glm::vec4(0.f));
            DESERIALIZE_VEC4(controlInputElement, "cmd_motor_thrusts", m_input.cmd_motor_thrusts, glm::vec4(0.f));
            DESERIALIZE_PROPERTY(controlInputElement, context, m_input.cmd_thrust);
            DESERIALIZE_VEC3(controlInputElement, "cmd_moment", m_input.cmd_moment, glm::vec3(0.f));
            DESERIALIZE_VEC4(controlInputElement, "cmd_q", m_input.cmd_q, glm::vec4(0.f));
            DESERIALIZE_VEC3(controlInputElement, "cmd_w", m_input.cmd_w, glm::vec3(0.f));
            DESERIALIZE_VEC3(controlInputElement, "cmd_v", m_input.cmd_v, glm::vec3(0.f));
            DESERIALIZE_VEC3(controlInputElement, "cmd_acc", m_input.cmd_acc, glm::vec3(0.f));
        }
    };

    [[nodiscard]] Version GetVersion() const override { return {1, 0, 0}; }

    // Getters for accessing the properties
    const quad_params& GetParams() const { return m_params; }
    quad_params& GetParams() { return m_params; }

    control_abstraction GetControlAbstraction() const { return m_control_abstraction; }
    void SetControlAbstraction(control_abstraction ca) { m_control_abstraction = ca; }

    const trajectory& GetTrajectory() const { return m_trajectory; }
    trajectory& GetTrajectory() { return m_trajectory; }

    const drone_state& GetDroneState() const { return m_drone_state; }
    drone_state& GetDroneState() { return m_drone_state; }

    const control_input& GetControlInput() const { return m_input; }
    control_input& GetControlInput() { return m_input; }

    private:
        quad_params m_params;
        control_abstraction m_control_abstraction;
        trajectory m_trajectory;
        drone_state m_drone_state;
        control_input m_input;
};