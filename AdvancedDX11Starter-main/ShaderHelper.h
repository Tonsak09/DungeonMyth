#pragma once

// Developer: Narai
// Purpose: Help with data structures to organize
//			entities' vertex and pixel shaders so
//			that 

#include <memory>
#include <DirectXMath.h>

#include "SimpleShader.h"
#include "Material.h"

#include "Lights.h"
#include "Transform.h"
#include "Camera.h"

// NOTE: To add a shader you must refer to it in the 
//		 following sections. 
//			A) In the PS/VS enum 
//			B) Add a function for setting it 
//			C) Include setting function in Set PS/VS function (Bottom of page)
//			D) Link to VS/PS in LoadAssetsAndCreateEntities()

// This should keep the adding of new shaders within this file
// and allow for the swapping of shader types to be easier 
// within game. 

// Also note that when loading a new shader to link it with the 
// appropriate shader enum 


enum PixelShaders
{
	COMMON,
	SOLID_COLOR,
	TRIPLANAR
};

enum VertexShaders
{
	VERTEX_SHADER,
	SHADOW_VERTEX,
};


static void LinkPSShader(
	const wchar_t* name, 
	PixelShaders link,
	std::unordered_map<const wchar_t*, PixelShaders>  psNameToID)
{

	if (psNameToID.find(name) == psNameToID.end())
	{
		// Add new shader combination 
		psNameToID.insert(
			std::pair<const wchar_t*, PixelShaders>
			(name, link));
	}
	else
	{
		// Replace 
		psNameToID[name] = link;
	}
}

#pragma region VERTEX_SHADERS

/// <summary>
/// Sends data for the VertexShader 
/// </summary>
static void SetVertexShader(
	std::shared_ptr<SimpleVertexShader> vs,
	Transform* transform,
	Camera* camera,
	DirectX::XMFLOAT4X4 shadowViewMatrix,
	DirectX::XMFLOAT4X4 shadowProjMatrix)
{
	vs->SetShader();

	// Send data to the vertex shader
	vs->SetMatrix4x4("world", transform->GetWorldMatrix());
	vs->SetMatrix4x4("worldInverseTranspose", transform->GetWorldInverseTransposeMatrix());
	vs->SetMatrix4x4("view", camera->viewMatrix);
	vs->SetMatrix4x4("projection", camera->projMatrix);
	vs->SetMatrix4x4("lightView", shadowViewMatrix);
	vs->SetMatrix4x4("lightProjection", shadowProjMatrix);
	vs->CopyAllBufferData();
}


#pragma endregion

#pragma region PIXEL_SHADERS



/// <summary>
/// Sends data universal to every material 
/// </summary>
static void SetMateralPixelData(
	std::shared_ptr<SimplePixelShader> ps,
	std::shared_ptr<RendMat> material)
{
	// TODO: Loop through material vector of pairs
	//		 and send data to shader. Should help 
	//		 make materials more generic 

	ps->SetFloat3("colorTint", material->colorTint);
	ps->SetFloat2("uvScale", material->uvScale);
	ps->SetFloat2("uvOffset", material->uvOffset);
	ps->CopyAllBufferData();
}

/// <summary>
/// Sends buffer data for the CommonPixel shader 
/// </summary>
static void SetCommonPixel(
	std::shared_ptr<RendMat> material,
	std::shared_ptr<SimplePixelShader> ps,
	Light dirLight,
	DirectX::XMFLOAT3 camPos,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowTextureSRV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler)
{
	// Common pixel shader from material 
	//std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
	ps->SetShader();

	// Set data specific to this shader 
	ps->SetData("worldLight", &dirLight, sizeof(Light));
	ps->SetFloat3("cameraPosition", camPos);
	ps->CopyBufferData("perFrame");

	// Set shadowmap shader which is passed in
	// every frame 
	ps->SetShaderResourceView("ShadowMap", shadowSRV);
	ps->SetSamplerState("ShadowSampler", shadowSampler);

	ps->SetShaderResourceView("ShadowTexture", shadowTextureSRV);

	// Set data 
	SetMateralPixelData(ps, material);
}

static void SetSolidColor(
	std::shared_ptr<RendMat> material,
	std::shared_ptr<SimplePixelShader> ps,
	Light dirLight,
	DirectX::XMFLOAT3 camPos,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler)
{
	// TODO...
}

static void SetTriplanar(
	std::shared_ptr<RendMat> material,
	std::shared_ptr<SimplePixelShader> ps,
	Light dirLight,
	DirectX::XMFLOAT3 camPos,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler)
{
	// Common pixel shader from material 
	ps->SetShader();

	// Set data specific to this shader 
	ps->SetData("worldLight", &dirLight, sizeof(Light));
	ps->SetFloat3("cameraPosition", camPos);
	ps->CopyBufferData("perFrame");

	// Set shadowmap shader which is passed in
	// every frame 
	ps->SetShaderResourceView("ShadowMap", shadowSRV);
	ps->SetSamplerState("ShadowSampler", shadowSampler);

	// Set data 
	PrepareMaterial(material, ps);
	SetMateralPixelData(ps, material);
}


/// <summary>
/// Sets up a lit pixel shader 
/// </summary>
static void SetPixelShader(
	std::shared_ptr<RendMat> material,
	std::shared_ptr<SimplePixelShader> ps,
	Light dirLight,
	DirectX::XMFLOAT3 camPos,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowTextureSRV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler,
	std::unordered_map<const wchar_t*, PixelShaders>  psNameToID)
{

	int type = psNameToID[material->psName];

	switch (type)
	{
	case COMMON:
		SetCommonPixel(material, ps, dirLight, camPos, shadowTextureSRV, shadowSRV, shadowSampler);
		break;
	case SOLID_COLOR:
		break;
	case TRIPLANAR:
		SetTriplanar(material, ps, dirLight, camPos, shadowSRV, shadowSampler);
		break;
	default:
		break;
	}
}

#pragma endregion 