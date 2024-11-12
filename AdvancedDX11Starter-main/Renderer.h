#pragma once

#include <d3d11.h>
#include <string>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

#include "Sky.h"

#include "ShaderHelper.h"
#include "GameEntity.h"

class Renderer
{
public:
	Renderer(
		// TODO: Generate internally 
		Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain,
		Microsoft::WRL::ComPtr<ID3D11Device>		device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context,

		bool vsync,
		bool deviceSupportsTearing,
		BOOL isFullscreen, 

		// TODO: Generate internally 
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowTextureSRV,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
		DirectX::XMFLOAT4X4 shadowViewMatrix,
		DirectX::XMFLOAT4X4 shadowProjectionMatrix,

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler,

		std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups,
		// TODO: Generate internally 
		std::unordered_map<const wchar_t*, std::shared_ptr<SimpleVertexShader>> nameToVS,
		std::unordered_map<const wchar_t*, std::shared_ptr<SimplePixelShader>> nameToPS,
		std::unordered_map<const wchar_t*, std::shared_ptr<RendMat>> nameToMat);
	~Renderer();

private:
	void DrawToTargetBuffer(
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetBuffer,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV, 
		std::shared_ptr<Camera> camPos);

	// DirectX 
	Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain;
	Microsoft::WRL::ComPtr<ID3D11Device>		device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context;

	// Screen settings
	bool vsync;
	bool deviceSupportsTearing;
	BOOL isFullscreen; // Due to alt+enter key combination (must be BOOL typedef)

	
	// Entity management 
	std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups;

	// Shaders
	std::unordered_map<const wchar_t*, std::shared_ptr<SimpleVertexShader>> nameToVS;
	std::unordered_map<const wchar_t*, std::shared_ptr<SimplePixelShader>> nameToPS;
	std::unordered_map<const wchar_t*, std::shared_ptr<RendMat>> nameToMat;

	std::unordered_map<const wchar_t*, PixelShaders>  psNameToID;
	std::unordered_map<const wchar_t*, VertexShaders> vsNameToID;

	// Lights
	Light dirLight; 

	// Shadow Mapping  
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;

	const int SHADOW_MAP_RESOLUTION = 2048;

	// Skybox
	std::shared_ptr<Sky> sky;
};