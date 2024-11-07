
float2 TileAndOffset(
    float2 uv,
    float2 tiling,
    float2 offset)
{
    return uv * tiling + offset;
}

float3 TriplanarTexture(
    float3 worldPos,
    float3 worldNormal,
    float sharpness,
    float2 frontTiling,
    float2 frontOffset,
    float2 sideTiling,
    float2 sideOffset,
    float2 topTiling,
    float2 topOffset,
    Texture2D frontTexture,
    Texture2D sideTexture,
    Texture2D topTexture,
    SamplerState basicSampler)
{
    // Get unique triplanar UVs based on normals
    float2 frontUV  = TileAndOffset(worldPos.xy,    frontTiling,   frontOffset);
    float2 sideUV   = TileAndOffset(worldPos.zy,    sideTiling,    sideOffset);
    float2 topUV    = TileAndOffset(worldPos.xz,    topTiling,     topOffset);
    
    // Sample from given textures  
    float3 front    = frontTexture.Sample(basicSampler, frontUV);
    float3 side     = sideTexture.Sample(basicSampler, sideUV);
    float3 top      = topTexture.Sample(basicSampler, topUV);
    
    // Normals used to determine weight of each texture
    // to interpolate from 
    float3 norm = pow(abs(worldNormal), sharpness);
    
    float base = norm.x + norm.y + norm.z;
    norm /= base;
    
    front   *= saturate(norm.z);
    side    *= saturate(norm.x);
    top     *= saturate(norm.y);
    
    return front + side + top;
}