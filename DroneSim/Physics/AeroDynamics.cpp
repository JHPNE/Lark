#include "Aerodynamics.h"
#include <cmath>

static const float AIR_DENSITY = 1.225f; // kg/m^3

void applyAeroAndPropForces(DroneData& drone, float deltaTime)
{
    if (!drone.body.fuselageBody) return;

    // (1) Multi-rotor or hybrid: rotor thrust
    if (drone.type == DroneType::MULTIROTOR || drone.type == DroneType::HYBRID)
    {
        for (int r = 0; r < drone.rotors.rotorCount; ++r)
        {
            float throttle = drone.rotors.rotorThrottle[r];
            float actualMaxThrust = drone.rotors.rotorMaxThrust;
            // If a rotor has failed
            if (drone.rotorFailFlag && r == drone.failRotorIndex)
                actualMaxThrust = 0.0f;

            float thrust = actualMaxThrust * throttle;

            btRigidBody* rotorBody = drone.body.childBodies[r];
            if (!rotorBody) continue;

            // Apply thrust in local +Y of the rotor
            btTransform rotorXform;
            rotorBody->getMotionState()->getWorldTransform(rotorXform);
            btVector3 up = rotorXform.getBasis() * btVector3(0,1,0);
            up.normalize();

            // Ground effect: if rotor is close to ground, increase effective thrust
            float groundDistance = rotorXform.getOrigin().y();
            float groundEffect = 1.0f;
            if (groundDistance < 2.0f)
            {
                // Simple model: 1 + factor*(1 - dist/2)
                groundEffect = 1.0f + (drone.groundEffectFactor - 1.0f) * (1.0f - groundDistance/2.0f);
                groundEffect = std::max(1.0f, groundEffect);
            }

            btVector3 force = up * thrust * groundEffect;
            rotorBody->applyCentralForce(force);

            // Propwash (placeholder): minor downward force on fuselage
            float propwashForce = thrust * 0.1f; 
            btVector3 down = -up;
            drone.body.fuselageBody->applyCentralForce(down * propwashForce);
        }
    }

    // (2) Fixed-wing or hybrid: lift & drag
    if (drone.type == DroneType::FIXED_WING || drone.type == DroneType::HYBRID)
    {
        btVector3 velocity = drone.body.fuselageBody->getLinearVelocity();
        float speed = velocity.length();
        if (speed < 1e-3f) return;

        float q = 0.5f * AIR_DENSITY * speed * speed; // dynamic pressure
        float lift  = q * drone.aeroDynamics.wingArea * drone.aeroDynamics.cLift;
        float drag  = q * drone.aeroDynamics.wingArea * drone.aeroDynamics.cDrag;

        // Orientation
        btTransform fuselageXform;
        drone.body.fuselageBody->getMotionState()->getWorldTransform(fuselageXform);
        
        // For a more realistic approach, compute angle of attack from pitch or local velocity vector
        // We'll assume the plane is aligned with x-forward, y-up, z-right in local space.
        // This is a huge simplification, but demonstrates the pattern:
        btVector3 forward = fuselageXform.getBasis() * btVector3(1,0,0);
        forward.normalize();
        btVector3 velDir = velocity.normalized();

        // If the drone is pointing somewhat forward, angleOfAttack ~ angle between forward & velDir
        float dotFV = forward.dot(velDir);
        float angleOfAttack = std::acos(dotFV);
        // Adjust lift/drag by AoA if desired
        // For example, cLift, cDrag = cLift0 + dLift/dAoA * angleOfAttack, etc.

        // Lift direction (assume perpendicular to velocity in local 'up' sense)
        btVector3 right = velDir.cross(btVector3(0,1,0));
        if (right.length2() < 1e-6f) right = btVector3(1,0,0);
        right.normalize();
        btVector3 localUp = right.cross(velDir);
        localUp.normalize();

        // Apply lift
        btVector3 liftForce = localUp * lift;
        // Apply drag
        btVector3 dragForce = -velDir * drag;

        drone.body.fuselageBody->applyCentralForce(liftForce + dragForce);
    }
}