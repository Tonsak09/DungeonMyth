#include "Material.h"

Material::Material(
	DirectX::XMFLOAT3 tint, 
	DirectX::XMFLOAT2 uvScale,
	DirectX::XMFLOAT2 uvOffset) 
	:
	colorTint(tint),
	uvScale(uvScale),
	uvOffset(uvOffset)
{

}

// Getters
DirectX::XMFLOAT2 Material::GetUVScale() { return uvScale; }
DirectX::XMFLOAT2 Material::GetUVOffset() { return uvOffset; }
DirectX::XMFLOAT3 Material::GetColorTint() { return colorTint; }

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetTextureSRV(std::string name)
{
	// Search for the key
	auto it = textureSRVs.find(name);

	// Not found, return null
	if (it == textureSRVs.end())
		return 0;

	// Return the texture ComPtr
	return it->second;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSampler(std::string name)
{
	// Search for the key
	auto it = samplers.find(name);

	// Not found, return null
	if (it == samplers.end())
		return 0;

	// Return the sampler ComPtr
	return it->second;
}

// Setters
void Material::SetUVScale(DirectX::XMFLOAT2 scale) { uvScale = scale; }
void Material::SetUVOffset(DirectX::XMFLOAT2 offset) { uvOffset = offset; }
void Material::SetColorTint(DirectX::XMFLOAT3 tint) { this->colorTint = tint; }


void Material::AddTextureSRV(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	textureSRVs.insert({ name, srv });
}

void Material::AddSampler(std::string name, Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler)
{
	samplers.insert({ name, sampler });
}

void Material::RemoveTextureSRV(std::string name)
{
	textureSRVs.erase(name);
}

void Material::RemoveSampler(std::string name)
{
	samplers.erase(name);
}

void Material::PrepareMaterial(std::shared_ptr<SimplePixelShader> inPS)
{
	// Loop and set any other resources
	for (auto& t : textureSRVs) { inPS->SetShaderResourceView(t.first.c_str(), t.second.Get()); }
	for (auto& s : samplers) { inPS->SetSamplerState(s.first.c_str(), s.second.Get()); }
}