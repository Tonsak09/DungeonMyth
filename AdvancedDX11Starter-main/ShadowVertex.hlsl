// Struct representing a single vertex worth of data
struct VertexShaderInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

// Constant Buffer for external (C++) data
cbuffer externalData : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
};

// --------------------------------------------------------
// A simplified vertex shader for rendering to a shadow map
// --------------------------------------------------------
float4 main(VertexShaderInput input) : SV_POSITION
{
    matrix wvp = mul(projection, mul(view, world));
    return mul(wvp, float4(input.position, 1.0f));
}