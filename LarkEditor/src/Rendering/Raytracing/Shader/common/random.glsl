uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float random(float f) {
    const uint mantissaMask = 0x007FFFFFu;
    const uint one          = 0x3F800000u;

    uint h = hash(floatBitsToUint(f));
    h &= mantissaMask;
    h |= one;

    float  r2 = uintBitsToFloat(h);
    return r2 - 1.0;
}

float randomFloat(float seed, float min, float max)
{
    return min + (max - min) * random(seed);
}

vec3 randomUnitVector()
{
    float baseSeed = dot(gl_FragCoord.xy, vec2(1.0, 4096.0));

    for (int i = 0; i < 8; ++i)
    {
        float seed = baseSeed + float(i) * 17.0;
        vec3 p = vec3(
        randomFloat(seed, -1.0, 1.0),
        randomFloat(seed + 1.0, -1.0, 1.0),
        randomFloat(seed + 2.0, -1.0, 1.0)
        );

        float len2 = dot(p, p);
        if (len2 > 1e-6 && len2 < 1.0)
        {
            return normalize(p);
        }
    }

    return normalize(vec3(0.0, 1.0, 0.0));
}

vec3 randomInHemisphere(vec3 normal)
{
    vec3 inUnitSphere = randomUnitVector();
    if (dot(inUnitSphere, normal) > 0.0)
    return inUnitSphere;
    else
    return -inUnitSphere;
}