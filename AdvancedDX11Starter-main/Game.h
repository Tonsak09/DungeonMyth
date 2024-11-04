#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "Sky.h"

#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <map>

#include "Player.h"

#include "DebugDrawManager.h"
#include "ShaderHelper.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void DrawShadowMap();
	void Draw(float deltaTime, float totalTime);

private:

	// Our scene
	std::vector<std::shared_ptr<GameEntity>> entities;
	std::vector<std::vector<std::shared_ptr<GameEntity>>> shaderSplitEntityList;
	bool updateMouseDelta; 

	// Player
	std::shared_ptr<PlayersData> playersData;

	// Lights
	std::vector<Light> lights;
	int lightCount;
	bool showPointLights;

	// These will be loaded along with other assets and
	// saved to these variables for ease of access
	std::shared_ptr<Mesh> lightMesh;
	std::shared_ptr<SimpleVertexShader> lightVS;
	std::shared_ptr<SimplePixelShader> lightPS;

	// Shaders 
	std::unordered_map<const wchar_t*, std::shared_ptr<SimpleVertexShader>> nameToVS;
	std::unordered_map<const wchar_t*, std::shared_ptr<SimplePixelShader>> nameToPS;
	std::unordered_map<const wchar_t*, std::shared_ptr<RendMat>> nameToMat;
	void AddVS(const wchar_t* name);
	void AddPS(const wchar_t* name);
	void AddMat(
		std::shared_ptr<RendMat> mat,
		const wchar_t* matName);

	// Entities grouped by shader type 
	// Group 1 is PixelShader
	// Group 2 is VertexShader
	std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups;
	void GroupEntitiesByShaders();
	void AddEntities();
	void RemoveEntities();


	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;

	// Skybox
	std::shared_ptr<Sky> sky;

	// Shadow Mapping  
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;

	const int SHADOW_MAP_RESOLUTION = 2048;

	


	// General helpers for setup and drawing
	void LoadAssetsAndCreateEntities();
	void GenerateLights();
	void GenerateShadowData();
	void DrawPointLights();
	void OnWorldLightChange(); 

	// UI functions
	void UINewFrame(float deltaTime);
	void BuildUI();
	void CameraUI(std::shared_ptr<FreeCamera> cam);
	void EntityUI(std::shared_ptr<GameEntity> entity);
	void LightUI(Light& light);
	
	// Should the ImGui demo window be shown?
	bool showUIDemoWindow;

	// Debug Drawing
	DebugDrawData debugDrawData; 
};

