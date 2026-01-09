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

    rec.hit = true;
    rec.t = t;
    rec.point = ray.origin + t * ray.direction;

    // Interpolate normal using barycentric coordinates (extract xyz from vec4)
    float w = 1.0 - u - v;
    rec.normal = normalize(w * tri.n0.xyz + u * tri.n1.xyz + v * tri.n2.xyz);

    // TODO: front face for glass

    // Interpolate UVs
    rec.uv = w * tri.uv0 + u * tri.uv1 + v * tri.uv2;
    rec.materialId = tri.materialId;

    return rec;
}