#version 410 core

out vec4 FragColor;
in vec2 TexCoords;

// Camera uniforms
uniform vec3 u_CameraPos;
uniform vec3 u_CameraFront;
uniform vec3 u_CameraUp;
uniform vec3 u_CameraRight;
uniform float u_Fov;
uniform float u_AspectRatio;
uniform vec2 u_Resolution;

uniform samplerBuffer u_TriangleData;
uniform int u_TriangleCount;

uniform samplerBuffer u_LightData;
uniform int u_LightCount;

uniform samplerBuffer u_MaterialData;
uniform int u_MaterialCount;

#include "common/structs.glsl"
#include "common/conversion.glsl"
#include "common/random.glsl"
#include "common/scatter.glsl"
#include "common/intersection.glsl"
#include "common/pathTracer.glsl"

void main()
{
    float seed = dot(gl_FragCoord.xy, vec2(1.0, 4096.0));
    float a = randomFloat(seed, -1.0, 1.0);
    float b = randomFloat(seed + 1, -1.0, 1.0);

    vec2 jitter = vec2(a, b) / u_Resolution;
    vec2 uv = TexCoords + jitter;

    Ray ray = generateRay(uv);

    int maxBounces = 5;
    vec3 color = pathTrace(ray, maxBounces);

    color = color / (color + vec3(1.0));

    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}