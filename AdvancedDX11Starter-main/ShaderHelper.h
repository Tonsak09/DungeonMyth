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

static void SetVertexShader(
	std::shared_ptr<SimpleVertexShader> vs,
	Transform* transform,
	Camera* camera)
{
	vs->SetShader();

	// Send data to the vertex shader
	vs->SetMatrix4x4("world", transform->GetWorldMatrix());
	vs->SetMatrix4x4("worldInverseTranspose", transform->GetWorldInverseTransposeMatrix());
	vs->SetMatrix4x4("view", camera->viewMatrix);
	vs->SetMatrix4x4("projection", camera->projMatrix);
	vs->CopyAllBufferData();
}

#pragma endregion

#pragma region PIXEL_SHADERS

static void SetMateral(
	std::shared_ptr<SimplePixelShader> ps,
	std::shared_ptr<Material> material)
{
	ps->SetFloat3("colorTint", material->GetColorTint());
	ps->SetFloat2("uvScale", material->GetUVScale());
	ps->SetFloat2("uvOffset", material->GetUVOffset());
	ps->CopyAllBufferData();

	// Sets the texture SRVs and smaples of the material 
	material->PrepareMaterial();

	// Loop and set any other resources
	//for (auto& t : material->GetTextureSRVs()) { ps->SetShaderResourceView(t.first.c_str(), t.second.Get()); }
	//for (auto& s : samplers) { ps->SetSamplerState(s.first.c_str(), s.second.Get()); }
}

static void SetCommonPixel(
	std::shared_ptr<Material> material,
	Light dirLight,
	DirectX::XMFLOAT3 camPos)
{
	// Common pixel shader from material 
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

	// Set data specific to this shader 
	ps->SetData("worldLight", &dirLight, sizeof(Light));
	ps->SetFloat3("cameraPosition", camPos);
	ps->CopyBufferData("perFrame"); 

	// Set data 
	SetMateral(ps, material);
}


#pragma endregion 