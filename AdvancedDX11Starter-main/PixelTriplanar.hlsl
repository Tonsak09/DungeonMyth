#include "Lighting.hlsli"
#include "Triplanar.hlsli"
#include "Common.hlsli"

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



Texture2D AlbedoFront   : register(t0);
Texture2D AlbedoSide    : register(t1);
Texture2D AlbedoTop     : register(t2);

Texture2D ShadowMap     : register(t3);

SamplerState BasicSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);


float4 main(VertexToPixel input) : SV_TARGET
{
    float2 tiling = float2(1, 1);
    float2 offset = float2(0, 0);
    
    //return float4((input.normal + float3(1, 1, 1)) / 2.0f, 1.0f);
    return float4(TriplanarTexture(
        input.worldPos,
        input.normal,
        64.0f,
        tiling,
        offset,
        tiling,
        offset,
        tiling,
        offset,
        AlbedoFront,
        AlbedoSide,
        AlbedoTop,
        BasicSampler), 1.0f);
    
    return AlbedoSide.Sample(BasicSampler, input.uv);
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}