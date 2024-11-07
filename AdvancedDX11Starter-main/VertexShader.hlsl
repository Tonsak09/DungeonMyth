
#include "Common.hlsli"

// Constant Buffer for external (C++) data
cbuffer externalData : register(b0)
{
	matrix world;
	matrix worldInverseTranspose;
	matrix view;
	matrix projection;
    matrix lightView;
    matrix lightProjection;
};

// Struct representing a single vertex worth of data
struct VertexShaderInput
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};


// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// --------------------------------------------------------
VertexToPixel main(VertexShaderInput input)
{
	// Set up output
	VertexToPixel output;

	// Calculate output position
	matrix worldViewProj = mul(projection, mul(view, world));
	output.screenPosition = mul(worldViewProj, float4(input.position, 1.0f));

	// Calculate the world position of this vertex (to be used
	// in the pixel shader when we do point/spot lights)
	output.worldPos = mul(world, float4(input.position, 1.0f)).xyz;

	// Make sure the other vectors are in WORLD space, not "local" space
	output.normal = normalize(mul((float3x3)worldInverseTranspose, input.normal));
	output.tangent = normalize(mul((float3x3)world, input.tangent)); // Tangent doesn't need inverse transpose!

	// Pass the UV through
	output.uv = input.uv;
	
	// Shadow map
    matrix shadowWVP = mul(lightProjection, mul(lightView, world));
    output.shadowMapPos = mul(shadowWVP, float4(input.position, 1.0f));
	
	return output;
}