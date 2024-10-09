
#include "Lighting.hlsli"


// Data that can change per material
cbuffer perMaterial : register(b0)
{
	// Surface color
    float3 colorTint;

	// UV adjustments
    float2 uvScale;
    float2 uvOffset;
};

// Data that only changes once per frame
cbuffer perFrame : register(b1)
{
	// Main directional light 
    Light worldLight;

	// Needed for specular (reflection) calculation
    float3 cameraPosition;
};


// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION; // The world position of this PIXEL
};


// Texture-related variables
Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
SamplerState BasicSampler : register(s0);


// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
	// Always re-normalize interpolated direction vectors
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);

	// Apply the uv adjustments
    input.uv = input.uv * uvScale + uvOffset;

	// Normal mapping
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
	
	// Treating roughness as a pseduo-spec map here
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float specPower = max(256.0f * (1.0f - roughness), 0.01f); // Ensure we never hit 0
	
	// Gamma correct the texture back to linear space and apply the color tint
    float4 surfaceColor = Albedo.Sample(BasicSampler, input.uv);
    surfaceColor.rgb = pow(surfaceColor.rgb, 2.2) * colorTint;

	// Total color for this pixel
    float3 totalColor = float3(0, 0, 0);

    totalColor += DirLight(worldLight, input.normal, input.worldPos, cameraPosition, specPower, surfaceColor.rgb);

	// Gamma correction
    return float4(pow(totalColor, 1.0f / 2.2f), 1);
}