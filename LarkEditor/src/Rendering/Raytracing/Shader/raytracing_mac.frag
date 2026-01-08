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

Ray generateRay(vec2 uv)
{
    Ray ray;
    ray.origin = u_CameraPos;
    vec2 ndc = uv * 2.0 - 1.0;
    float tanHalfFov = tan(radians(u_Fov * 0.5));

    ray.direction = normalize(
    u_CameraFront +
    ndc.x * tanHalfFov * u_AspectRatio * u_CameraRight +
    ndc.y * tanHalfFov * u_CameraUp
    );

    return ray;
}

// Möller–Trumbore intersection algorithm
HitRecord intersectTriangle(Ray ray, int triIndex)
{
    Triangle tri = getTriangle(triIndex);

    HitRecord rec;
    rec.hit = false;
    rec.t = 1e30;

    const float EPS = 1e-8;

    vec3 v0 = tri.v0.xyz;
    vec3 v1 = tri.v1.xyz;
    vec3 v2 = tri.v2.xyz;

    vec3 E1 = v1 - v0;
    vec3 E2 = v2 - v0;
    vec3 P = cross(ray.direction, E2);

    float det = dot(E1, P);
    if (abs(det) < EPS) return rec;

    float invDet = 1.0 / det;
    vec3 T = ray.origin - v0;

    float u = dot(T, P) * invDet;
    if (u < 0.0 || u > 1.0) return rec;

    vec3 Q = cross(T, E1);
    float v = dot(ray.direction, Q) * invDet;
    if (v < 0.0 || u + v > 1.0) return rec;

    float t = dot(E2, Q) * invDet;
    if (t < 0.0) return rec;

    // Hit found
    rec.hit = true;
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;

    // Interpolate normal using barycentric coordinates (extract xyz from vec4)
    float w = 1.0 - u - v;
    rec.normal = normalize(w * tri.n0.xyz + u * tri.n1.xyz + v * tri.n2.xyz);

    // Interpolate UVs
    rec.uv = w * tri.uv0 + u * tri.uv1 + v * tri.uv2;
    rec.materialId = tri.materialId;

    return rec;
}

// Trace ray against all triangles and find closest hit
HitRecord traceRay(Ray ray)
{
    HitRecord closestHit;
    closestHit.hit = false;
    closestHit.t = 1e30;

    for (int i = 0; i < u_TriangleCount; i++)
    {
        HitRecord hit = intersectTriangle(ray, i);
        if (hit.hit && hit.t < closestHit.t)
        {
            closestHit = hit;
        }
    }

    return closestHit;
}

vec3 backgroundColor(Ray ray)
{
    vec3 unitDir = normalize(ray.direction);
    float t = 0.5 * (unitDir.y + 1.0);
    vec3 skyBlue = vec3(0.5, 0.7, 1.0);
    vec3 horizon = vec3(1.0, 1.0, 1.0);
    return mix(horizon, skyBlue, t);
}

bool lightIntersects(vec3 hitPoint, Light light)
{
    vec3 toLight = normalize(light.pos - hitPoint);
    float alignment = dot(normalize(-light.dir), toLight);

    // if we have a cone kinda light we need an angle
    // for now just ignore it ig
    float coneAngle = 0.9;

    if (alignment < coneAngle)
    {
        return false;
    }

    return true;
}

/** SHADING PART USING PBR https://learnopengl.com/PBR/Lighting **/

/** Helpers for shading **/
const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 shade(HitRecord rec)
{
    Material mat = getMaterial(rec.materialId);

    vec3 N = rec.normal;
    vec3 V = normalize(u_CameraPos - rec.point);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, mat.albedo, mat.metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++)
    {
        Light light = getLight(i);
        vec3 L = normalize(light.pos - rec.point);
        vec3 H = normalize(V + L);
        float distance = length(light.pos - rec.point);
        float attentutation = 1.0 / (distance * distance);
        vec3 radiance = light.color * attentutation;

        // Cook-torrance brdf
        float NDF = DistributionGGX(N, H, mat.roughness);
        float G = GeometrySmith(N, V, L, mat.roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - mat.metallic;

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator/denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * mat.albedo / PI + specular) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * mat.albedo * mat.ao;
    return ambient + Lo;
}

void main()
{
    Ray ray = generateRay(TexCoords);

    HitRecord rec = traceRay(ray);

    vec3 color;
    if (rec.hit)
    {
        color = shade(rec);
        color = color / (color + vec3(1.0));
    }
    else
    {
        color = backgroundColor(ray);
    }

    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}