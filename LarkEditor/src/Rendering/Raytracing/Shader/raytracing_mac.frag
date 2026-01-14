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
uniform int u_SamplesPerPixel;
uniform float u_FrameSeed;

uniform samplerBuffer u_TriangleData;
uniform int u_TriangleCount;

uniform samplerBuffer u_MaterialData;
uniform int u_MaterialCount;

// Per-sample seed that is set inside main's SPP loop
float g_SampleSeed = 0.0;

#include "common/structs.glsl"
#include "common/conversion.glsl"
#include "common/reflection.glsl"
#include "common/scatter.glsl"
#include "common/intersection.glsl"
#include "common/pathTracer.glsl"

void main()
{
    int spp = max(u_SamplesPerPixel, 1);
    vec3 accumulated = vec3(0.0);

    for (int i = 0; i < spp; ++i)
    {
        // Unique seed per sample to decorrelate noise
        g_SampleSeed = u_FrameSeed + float(i) * 9176.0;

        float seed = dot(gl_FragCoord.xy, vec2(1.0, 4096.0)) + g_SampleSeed;
        float a = randomFloat(seed, -1.0, 1.0);
        float b = randomFloat(seed + 1.0, -1.0, 1.0);

        vec2 jitter = vec2(a, b) / u_Resolution;
        vec2 uv = TexCoords + jitter;

        Ray ray = generateRay(uv);

        int maxBounces = 5;
        vec3 color = pathTrace(ray, maxBounces);

        color = color / (color + vec3(1.0));

        color = pow(color, vec3(1.0/2.2));

        accumulated += color;
    }

    vec3 finalColor = accumulated / float(spp);
    FragColor = vec4(finalColor, 1.0);
}