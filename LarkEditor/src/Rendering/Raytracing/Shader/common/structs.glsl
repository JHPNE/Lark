struct Triangle
{
    vec3 v0, v1, v2;
    vec3 n0, n1, n2;
    vec2 uv0, uv1, uv2;
    uint materialId;
};

struct Light
{
    vec3 pos;
    vec3 dir;
    vec3 color;
    float intsensity;
    float radius;
};

struct Material
{
    vec3 albedo;
    float roughness;
    float metallic;
    vec3 normal;
    float ao;
    vec3 emissive;
    float ior;
    float transparency;
};

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct HitRecord
{
    float t;
    vec3 point;
    vec3 normal;
    vec2 uv;
    uint materialId;
    bool hit;
};
