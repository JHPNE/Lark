uint randNumb;

uint hash() {
    randNumb += (randNumb << 10u);
    randNumb ^= (randNumb >>  6u);
    randNumb += (randNumb <<  3u);
    randNumb ^= (randNumb >> 11u);
    randNumb += (randNumb << 15u);
    return randNumb;
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
    while (true)
    {

        float seed = dot(gl_FragCoord.xy, vec2(1.0, 4096.0));
        vec3 p = vec3(
        randomFloat(seed, -1.0, 1.0),
        randomFloat(seed + 1, -1.0, 1.0),
        randomFloat(seed + 2, -1.0, 1.0)
        );
        if (1e-160< dot(p, p) && dot(p, p) < 1.0)
        return normalize(p);
    }
}

vec3 randomInHemisphere(vec3 normal)
{
    vec3 inUnitSphere = randomUnitVector();
    if (dot(inUnitSphere, normal) > 0.0)
    return inUnitSphere;
    else
    return -inUnitSphere;
}