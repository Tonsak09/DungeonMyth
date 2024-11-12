#pragma once

#include <memory>
#include <wrl/client.h>

#include "SimpleShader.h"
#include "ShaderHelper.h"


struct LightProbe
{
	// TODO: Convert the cube map into a single image
	//		 using spherical harmonics 

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;
	DirectX::XMFLOAT3 position;
};

/// <summary>
/// Iterate through the static level geometry and generate the 
/// light probes based on cube map photos and storing as sphereical
/// harmonics. Uses previously generated light data for each update 
/// </summary>
void BuildLightProbes()
{

}

std::shared_ptr<LightProbe> GenerateLightProbe()
{
	// Set manual draw 


}