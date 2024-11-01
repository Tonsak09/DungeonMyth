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


enum PixelShaders
{
	COMMON
};


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

	// Set data 
	SetMateralPixelData(ps, material);
}


#pragma endregion 