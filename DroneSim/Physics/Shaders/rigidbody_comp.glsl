#version 430

layout(local_size_x = 64) in; //Work group size

// Positions
layout(std430, binding = 0) buffer PositionBuffer {
    vec4 positions[];
};

// Orientations
layout(std430, binding = 1) buffer OrientationBuffer {
    vec4 orientations[];
};

// Linear velocities (vec4): xyz velocity, w = unused
layout(std430, binding = 2) buffer LinearVelocityBuffer {
    vec4 linearVelocities[];
};

// Angular velocities (vec4): xyz angular velocity, w = unused
layout(std430, binding = 3) buffer AngularVelocityBuffer {
    vec4 angularVelocities[];
};

// Mass data (vec4): x = mass, y = invMass, z = inertia.x, w = inertia.y
layout(std430, binding = 4) buffer MassBuffer {
    vec4 massData[];
};

// Inertia data (vec4): x = inertia.z, y = invInertia.x, z = invInertia.y, w = invInertia.z
layout(std430, binding = 5) buffer InertiaBuffer {
    vec4 inertiaData[];
};

uniform float dt;
uniform vec3 gravity;

void main() {
    uint i = gl_GlobalInvocationID.x;

    // Load data
    vec4 pos = positions[i];
    vec4 ori = orientations[i];
    vec4 linVel = linearVelocities[i];
    vec4 angVel = angularVelocities[i];

    vec4 mData = massData[i];
    vec4 iData = inertiaData[i];

    float mass = mData.x;
    float invMass = mData.y;
    // Inertia diagonals (for simplicity)
    // mData.z, mData.w and iData.x for the remaining inertia components
    // Let’s say:
    // massData[i].z = inertia.x
    // massData[i].w = inertia.y
    // inertiaData[i].x = inertia.z
    // inertiaData[i].y = invInertia.x
    // inertiaData[i].z = invInertia.y
    // inertiaData[i].w = invInertia.z
    vec3 inertia = vec3(mData.z, mData.w, iData.x);
    vec3 invInertia = vec3(iData.y, iData.z, iData.w);

    if (invMass > 0.0) {
        // Semi-implicit Euler integration

        // 1) Update linear velocity with gravity
        linVel.xyz += gravity * dt;

        // 2) Update angular velocity (no external torque in this example, but you could apply some if needed)
        // Here we do nothing special, but if you had torques, you'd do:
        // angVel.xyz += invInertia * torque * dt; // For diagonal inertia, this is element-wise multiply.

        // 3) Update position
        pos.xyz += linVel.xyz * dt;

        // 4) Update orientation using angular velocity:
        // Orientation integration:
        // q_dot = 0.5 * omega_quat * q
        // omega_quat = (0, angVel)
        vec4 q = ori;
        vec3 wv = angVel.xyz;
        vec4 wq = vec4(0.0, wv.x, wv.y, wv.z);

        vec4 qDot = 0.5 * vec4(
            wq.w*q.w - wq.x*q.x - wq.y*q.y - wq.z*q.z,
            wq.w*q.x + wq.x*q.w + wq.y*q.z - wq.z*q.y,
            wq.w*q.y - wq.x*q.z + wq.y*q.w + wq.z*q.x,
            wq.w*q.z + wq.x*q.y - wq.y*q.x + wq.z*q.w
        );

        vec4 newQ = q + qDot * dt;

        // Normalize quaternion
        float len = inversesqrt(newQ.x*newQ.x + newQ.y*newQ.y + newQ.z*newQ.z + newQ.w*newQ.w);
        newQ *= len;

        // Store back
        positions[i] = pos;
        orientations[i] = newQ;
        linearVelocities[i] = linVel;
        angularVelocities[i] = angVel;
    }
}