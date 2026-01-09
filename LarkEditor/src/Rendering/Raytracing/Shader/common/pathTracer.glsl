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

vec3 pathTrace(Ray ray, int maxBounces)
{
    vec3 color = vec3(1.0);
    Ray currentRay = ray;

    for (int bounce = 0; bounce < maxBounces; bounce ++)
    {
        HitRecord rec = traceRay(currentRay);

        if (!rec.hit)
        {
            return color * backgroundColor(currentRay);
        }

        Material mat = getMaterial(rec.materialId);

        vec3 attenuation;
        Ray scattered;
        bool didScatter = false;

        // Currently will default to lamb
        // TODO: Add the dielectric and metal
        int matType = int(mat.type);

        didScatter = scatterLambertian(mat, rec, attenuation, scattered);

        if (!didScatter)
        {
            return vec3(0.0);
        }

        color *= attenuation;
        currentRay = scattered;
    }

    return vec3(0.0);
}