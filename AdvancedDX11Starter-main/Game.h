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

#include "Player.h"

#include "DebugDrawManager.h"

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
	void Draw(float deltaTime, float totalTime);

private:

	// Our scene
	std::vector<std::shared_ptr<GameEntity>> entities;
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

	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;

	// Skybox
	std::shared_ptr<Sky> sky;

	// General helpers for setup and drawing
	void LoadAssetsAndCreateEntities();
	void GenerateLights();
	void DrawPointLights();

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

