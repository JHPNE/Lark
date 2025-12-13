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

struct Triangle {
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec2 uv0, uv1, uv2;
    uint materialId;
};

// HELPER
vec3 readVec3(int baseIndex, int offset)
{
    // Each vec3 needs to be stored as vec4 in the TBO (RGBA format)
    // So we read the RGB components
    return texelFetch(u_TriangleData, baseIndex + offset).xyz;
}

vec2 readVec2(int baseIndex, int offset)
{
    return texelFetch(u_TriangleData, baseIndex + offset).xy;
}

uint readUint(int baseIndex, int offset)
{
    return floatBitsToUint(texelFetch(u_TriangleData, baseIndex + offset).x);
}

Triangle getTriangle(int index)
{
    Triangle tri;

    // Calculate base index for this triangle
    // Each triangle needs: 6 vec4s for positions/normals + 2 vec4s for UVs/material
    // = 8 vec4s per triangle
    int baseIndex = index * 8;

    tri.v0 = readVec3(baseIndex, 0);
    tri.v1 = readVec3(baseIndex, 1);
    tri.v2 = readVec3(baseIndex, 2);
    tri.n0 = readVec3(baseIndex, 3);
    tri.n1 = readVec3(baseIndex, 4);
    tri.n2 = readVec3(baseIndex, 5);

    vec4 uvData0 = texelFetch(u_TriangleData, baseIndex + 6);
    vec4 uvData1 = texelFetch(u_TriangleData, baseIndex + 7);

    tri.uv0 = uvData0.xy;
    tri.uv1 = uvData0.zw;
    tri.uv2 = uvData1.xy;
    tri.materialId = floatBitsToUint(uvData1.z);

    return tri;
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    float t;
    vec3 point;
    vec3 normal;
    vec2 uv;
    uint materialId;
    bool hit;
};

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

void main()
{
    Ray ray = generateRay(TexCoords);

    HitRecord rec = traceRay(ray);

    vec3 color;
    if (rec.hit)
    {
        // Visualize normal (can be changed to material-based shading later)
        color = rec.normal * 0.5 + 0.5;
    }
    else
    {
        color = backgroundColor(ray);
    }

    FragColor = vec4(color, 1.0);
}