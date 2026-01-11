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