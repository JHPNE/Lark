Triangle getTriangle(int index)
{
    Triangle tri;
    int baseIndex = index * 8;

    tri.v0 = texelFetch(u_TriangleData, baseIndex + 0).xyz;
    tri.v1 = texelFetch(u_TriangleData, baseIndex + 1).xyz;
    tri.v2 = texelFetch(u_TriangleData, baseIndex + 2).xyz;
    tri.n0 = texelFetch(u_TriangleData, baseIndex + 3).xyz;
    tri.n1 = texelFetch(u_TriangleData, baseIndex + 4).xyz;
    tri.n2 = texelFetch(u_TriangleData, baseIndex + 5).xyz;

    vec4 uvData0 = texelFetch(u_TriangleData, baseIndex + 6);
    vec4 uvData1 = texelFetch(u_TriangleData, baseIndex + 7);

    tri.uv0 = uvData0.xy;
    tri.uv1 = uvData0.zw;
    tri.uv2 = uvData1.xy;
    tri.materialId = floatBitsToUint(uvData1.z);

    return tri;
}

Material getMaterial(uint index)
{
    Material mat;
    int baseIndex = int(index) * 4;

    vec4 data0 = texelFetch(u_MaterialData, baseIndex + 0);
    vec4 data1 = texelFetch(u_MaterialData, baseIndex + 1);
    vec4 data2 = texelFetch(u_MaterialData, baseIndex + 2);
    vec4 data3 = texelFetch(u_MaterialData, baseIndex + 3);

    mat.albedo = data0.xyz;
    mat.roughness = data0.w;
    mat.normal = data1.xyz;
    mat.ao = data1.w;
    mat.emissive = data2.xyz;
    mat.ior = data2.w;
    mat.transparency = data3.x;
    mat.metallic = data3.y;
    mat.type = int(data3.z);

    return mat;
}