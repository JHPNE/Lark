bool scatterLambertian(Material mat, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    vec3 scatterDir = randomInHemisphere(rec.normal);

    // 0.001 prevents shadow acne
    scattered.origin = rec.point + rec.normal * 0.001;
    scattered.direction = normalize(scatterDir);
    attenuation = mat.albedo;

    return true;
}