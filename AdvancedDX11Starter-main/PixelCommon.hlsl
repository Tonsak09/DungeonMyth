
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
    float4 shadowMapPos : SHADOW_POSITION;
};


// Texture-related variables
Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D ShadowMap : register(t3);
SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);

// Entry point for this pixel shader
float4 main(VertexToPixel input) : SV_TARGET
{
    
    // Shadows
    
    // Perform the perspective divide (divide by W) ourselves
    input.shadowMapPos /= input.shadowMapPos.w;
    
    // Convert the normalized device coordinates to UVs for sampling
    float2 shadowUV = input.shadowMapPos.xy * 0.5f + 0.5f;
    shadowUV.y = 1 - shadowUV.y; // Flip the Y
    
    // Grab the distances we need: light-to-pixel and closest-surface
    float distToLight = input.shadowMapPos.z;
    float distShadowMap = ShadowMap.Sample(BasicSampler, shadowUV).r;
    
    // Get a ratio of comparison results using SampleCmpLevelZero()
    float shadowAmount = ShadowMap.SampleCmpLevelZero(
        ShadowSampler,
        shadowUV,
        distToLight).r;
    shadowAmount = max(0.01f, shadowAmount);
    
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

    totalColor += shadowAmount * DirLight(worldLight, input.normal, input.worldPos, cameraPosition, specPower, surfaceColor.rgb);
    totalColor = HeightFogColor(10.0f, 15.0f, -2.0f, -3.0f, cameraPosition, input.worldPos, totalColor, float3(0.2f, 0.2f, 0.25f));
    
	// Gamma correction
    return float4(pow(totalColor, 1.0f / 2.2f), 1);
}