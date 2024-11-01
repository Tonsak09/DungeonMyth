#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>

#include "SimpleShader.h"
#include "Camera.h"
#include "Transform.h"

class Material
{
public:
	Material(
		DirectX::XMFLOAT3 tint, 
		DirectX::XMFLOAT2 uvScale = DirectX::XMFLOAT2(1, 1),
		DirectX::XMFLOAT2 uvOffset = DirectX::XMFLOAT2(0, 0));

	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();
	DirectX::XMFLOAT3 GetColorTint();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTextureSRV(std::string name);
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSampler(std::string name);

	void SetUVScale(DirectX::XMFLOAT2 scale);
	void SetUVOffset(DirectX::XMFLOAT2 offset);
	void SetColorTint(DirectX::XMFLOAT3 tint);

	void AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv);
	void AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler);

	void RemoveTextureSRV(std::string name);
	void RemoveSampler(std::string name);

	void PrepareMaterial(std::shared_ptr<SimplePixelShader> inPS);


private:

	// Material properties
	DirectX::XMFLOAT3 colorTint;

	// Texture-related
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;
};


struct RendMat
{

	RendMat(
		DirectX::XMFLOAT3 _colorTint,
		DirectX::XMFLOAT2 _uvOffset,
		DirectX::XMFLOAT2 _uvScale,
		const wchar_t* _vsName,
		const wchar_t* _psName) :
		colorTint(_colorTint),
		uvOffset(_uvOffset),
		uvScale(_uvScale),
		vsName(_vsName),
		psName(_psName) { }

	RendMat(
		DirectX::XMFLOAT3 _colorTint,
		DirectX::XMFLOAT2 _uvOffset,
		DirectX::XMFLOAT2 _uvScale) :
		colorTint(_colorTint),
		uvOffset(_uvOffset),
		uvScale(_uvScale) { }

	// TODO: Make materials more generic with their data 
	// Material universal properties
	DirectX::XMFLOAT3 colorTint;

	// Texture-related
	DirectX::XMFLOAT2 uvOffset;
	DirectX::XMFLOAT2 uvScale;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>> samplers;

	const wchar_t* vsName;
	const wchar_t* psName;
};

/// <summary>
/// Add a textureSRV to the given RendMat 
/// </summary>
inline static void AddTextureSRV(
	std::shared_ptr<RendMat> mat,
	std::string name,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	mat->textureSRVs.insert(
		std::pair<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>
		(name, srv));
}

/// <summary>
/// Add a sampler to the given RendMat
/// </summary>
inline static void AddSampler(
	std::shared_ptr<RendMat> mat,
	std::string name,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	mat->samplers.insert(
		std::pair<std::string, Microsoft::WRL::ComPtr<ID3D11SamplerState>>
		(name, sampler));
}
 	    
/// <summary>
/// Remove a textureSRV from the given
/// RendMat by name 
/// </summary>
inline static void RemoveTextureSRV(
	std::shared_ptr<RendMat> mat,
	std::string name)
{
	mat->textureSRVs.erase(name);
}

/// <summary>
/// Remove a sampler from the given 
/// RendMat by name 
/// </summary>
inline static void RemoveSampler(
	std::shared_ptr<RendMat> mat,
	std::string name)
{
	mat->textureSRVs.erase(name);
}
 	    
/// <summary>
/// Prepares the TextureSRVs and Samplers
/// for the rending of the entity 
/// </summary>
/// <param name="mat"></param>
/// <param name="inPS"></param>
inline static void PrepareMaterial(
	std::shared_ptr<RendMat> mat,
	std::shared_ptr<SimplePixelShader> inPS)
{
	// Loop and set any other resources
	for (auto& t : mat->textureSRVs) { inPS->SetShaderResourceView(t.first.c_str(), t.second.Get()); }
	for (auto& s : mat->samplers) { inPS->SetSamplerState(s.first.c_str(), s.second.Get()); }
}