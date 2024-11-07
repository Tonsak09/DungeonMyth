// Out of the vertex shader (and eventually input to the PS)
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION; // The world position of this vertex
    float4 shadowMapPos : SHADOW_POSITION;
};
