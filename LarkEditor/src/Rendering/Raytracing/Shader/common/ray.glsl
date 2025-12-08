// Camera uniforms
uniform vec3 u_CameraPos;
uniform vec3 u_CameraFront;
uniform vec3 u_CameraUp;
uniform vec3 u_CameraRight;
uniform float u_Fov;
uniform float u_AspectRatio;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

Ray generateRay(vec2 uv)
{
    Ray ray;
    ray.origin = u_CameraPos;

    vec2 ndc = uv * 2.0 - 1.0;
    float tanHalfFov = tan(radians(u_Fov * 0.5));

    vec3 rayDir = normalize(
    u_CameraFront +
    ndc.x * tanHalfFov * u_AspectRatio * u_CameraRight +
    ndc.y * tanHalfFov * u_CameraUp
    );

    ray.direction = rayDir;
    return ray;
}