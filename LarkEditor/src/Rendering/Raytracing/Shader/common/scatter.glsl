bool scatterLambertian(Material mat, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    vec3 scatterDir = randomInHemisphere(rec.normal);

    // 0.001 prevents shadow acne
    scattered.origin = rec.point + rec.normal * 0.001;

    if (dot(scatterDir, scatterDir) < 1e-6)
    {
        scatterDir = rec.normal;
    }


    scattered.direction = normalize(scatterDir);
    attenuation = mat.albedo;

    return true;
}

bool scatterMetal(Material mat, Ray rayIn, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    vec3 reflected = reflect(normalize(rayIn.direction), rec.normal);
    scattered.origin = rec.point + rec.normal * 0.001;
    scattered.direction = normalize(reflected + mat.roughness * randomInHemisphere(rec.normal));
    attenuation = mat.albedo;
    return (dot(scattered.direction, rec.normal) > 0.0);
}

bool scatterDielectric(Material mat, Ray rayIn, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    attenuation = vec3(1.0);
    float ri = rec.frontFace ? (1.0/mat.ior) : mat.ior;

    vec3 unitDirection = normalize(rayIn.direction);
    float cosTheta = min(dot(-unitDirection, rec.normal), 1.0);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    bool cannotRefract = ri * sinTheta > 1.0;

    vec3 direction;
    if (cannotRefract || reflectance(cosTheta, ri) > randomFloat(dot(gl_FragCoord.xy, vec2(1.0, 4096.0)) + g_SampleSeed, -1.0, 1.0))
    {
        direction = reflect(unitDirection, rec.normal);
    }
    else
    {
        direction = refract(unitDirection, rec.normal, ri);
    }


    float bias = 0.001;
    if (cannotRefract || reflectance(cosTheta, ri) >
    randomFloat(dot(gl_FragCoord.xy, vec2(1.0, 4096.0)) + 69.0 + g_SampleSeed, -1.0, 1.0))
    {
        scattered.origin = rec.point + rec.normal * bias;
    }
    else
    {
        scattered.origin = rec.point - rec.normal * bias;
    }
    scattered.direction = direction;

    return true;
}