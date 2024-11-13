
#include <stdlib.h>     // For seeding random and rand()
#include <time.h>       // For grabbing time (to seed random)

#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "Helpers.h"

#include "WICTextureLoader.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"


// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Helper macro for getting a float between min and max
#define RandomRange(min, max) (float)rand() / RAND_MAX * (max - min) + min

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) std::make_shared<type>(device.Get(), context.Get(), FixPath(file).c_str()) 


// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"Dungeon Myth",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),				// Show extra stats (fps) in title bar?
	sky(0),
	lightCount(0),
	showUIDemoWindow(false),
	showPointLights(false),
	updateMouseDelta(true)
{
	// Seed random
	srand((unsigned int)time(0));

	playersData = std::make_shared<PlayersData>();



#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object

	// ImGui clean up
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Initialize ImGui itself & platform/renderer backends
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());
	ImGui::StyleColorsDark();

	// Add Debug Drawer 
	debugDrawData = DebugDrawData(context, device);

	// Asset loading and entity creation
	LoadAssetsAndCreateEntities();
	
	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set up lights initially
	lightCount = 64;
	GenerateLights();
	OnWorldLightChange();

	// Set up shadow map resources 
	GenerateShadowData();

	// Set initial graphics API state
	//  - These settings persist until we change them
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}



	// Set initial player data 
	AddPlayer(playersData.get(), "Eureka", (float)windowWidth / (float)windowHeight);
	playersData->transforms[0].SetPosition(0.0f, 0.0f, -10.0f);


	renderer = std::make_shared<Renderer>(
		swapChain,
		device,
		context,
		vsync,
		deviceSupportsTearing,
		shadowDSV,
		shadowTextureSRV,
		shadowSRV,
		shadowViewMatrix,
		shadowProjectionMatrix,
		shadowRasterizer,
		shadowSampler,
		entityGroups,
		entities,
		sky,
		nameToVS,
		nameToPS,
		nameToMat);

	OnResize();
	renderer->DrawToTargetBuffer(backBufferRTV, depthBufferDSV, &playersData->cams[0]);
}

/// <summary>
/// Stores a vertex shader into the game 
/// </summary>
/// <param name="name"></param>
void Game::AddVS(const wchar_t* name, VertexShaders linkedEnum)
{
	std::shared_ptr<SimpleVertexShader> vertexShader = 
		LoadShader(SimpleVertexShader, name);
	
	nameToVS.insert(std::pair<const wchar_t*, std::shared_ptr<SimpleVertexShader>>(
		name, vertexShader));
}

/// <summary>
/// Stores a pixel shader into the game 
/// </summary>
/// <param name="name"></param>
void Game::AddPS(const wchar_t* name, PixelShaders linkedEnum)
{
	std::shared_ptr<SimplePixelShader> pixelShader =
		LoadShader(SimplePixelShader, name);

	nameToPS.insert(std::pair<const wchar_t*, std::shared_ptr<SimplePixelShader>>(
		name, pixelShader));

	if (psNameToID.find(name) == psNameToID.end())
	{
		// Add new shader combination 
		psNameToID.insert(
			std::pair<const wchar_t*, PixelShaders>
			(name, linkedEnum));
	}
	else
	{
		// Replace 
		psNameToID[name] = linkedEnum;
	}
}

/// <summary>
/// Adds a material into the game 
/// </summary>
/// <param name="mat"></param>
/// <param name="matName"></param>
void Game::AddMat(
	std::shared_ptr<RendMat> mat,
	const wchar_t* matName)
{
	nameToMat.insert(
		std::pair<const wchar_t*, std::shared_ptr<RendMat>>(
		matName, mat));
}

// --------------------------------------------------------
// Load all assets and create materials, entities, etc.
// --------------------------------------------------------
void Game::LoadAssetsAndCreateEntities()
{
	psNameToID = std::unordered_map<const wchar_t*, PixelShaders>();
	vsNameToID = std::unordered_map<const wchar_t*, VertexShaders>();

	// Load active shaders 
	AddVS(	L"VertexShader.cso",		VERTEX_SHADER		);
	AddVS(	L"ShadowVertex.cso",		SHADOW_VERTEX		);
	AddPS(	L"PixelCommon.cso",			COMMON				);
	AddPS(	L"SolidColorPS.cso",		SOLID_COLOR			);
	AddPS(	L"PixelTriplanar.cso",		TRIPLANAR			);
	AddPS(	L"TriplanarShadows.cso",	TRIPLANAR_SHADOWS	);

	// Shaders only needed here 
	std::shared_ptr<SimpleVertexShader> skyVS = LoadShader(SimpleVertexShader, L"SkyVS.cso");
	std::shared_ptr<SimplePixelShader> skyPS  = LoadShader(SimplePixelShader, L"SkyPS.cso");

	// Make the meshes
	std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);
	std::shared_ptr<Mesh> helixMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str(), device);
	std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str(), device);
	std::shared_ptr<Mesh> coneMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cone.obj").c_str(), device);
	std::shared_ptr<Mesh> planeMesh = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/plane.obj").c_str(), device);
	std::shared_ptr<Mesh> sampleLevel = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/SampleLevel.obj").c_str(), device);
	std::shared_ptr<Mesh> skelly = std::make_shared<Mesh>(FixPath(L"../../Assets/Models/Skelly.obj").c_str(), device);
	
	// Declare the textures we'll need
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cobbleA,  cobbleN,  cobbleR,  cobbleM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> floorA,  floorN,  floorR,  floorM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> paintA,  paintN,  paintR,  paintM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> scratchedA,  scratchedN,  scratchedR,  scratchedM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> bronzeA,  bronzeN,  bronzeR,  bronzeM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughA,  roughN,  roughR,  roughM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> woodA,  woodN,  woodR,  woodM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> heronA, heronN, heronR, heronM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wandA, wandN, wandR, wandM;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> triFront, triSide, triTop;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noise;

	// Load the textures using our succinct LoadTexture() macro
	LoadTexture(L"../../Assets/Textures/cobblestone_albedo.png", cobbleA);
	LoadTexture(L"../../Assets/Textures/cobblestone_normals.png", cobbleN);
	LoadTexture(L"../../Assets/Textures/cobblestone_roughness.png", cobbleR);
	LoadTexture(L"../../Assets/Textures/cobblestone_metal.png", cobbleM);

	LoadTexture(L"../../Assets/Textures/floor_albedo.png", floorA);
	LoadTexture(L"../../Assets/Textures/floor_normals.png", floorN);
	LoadTexture(L"../../Assets/Textures/floor_roughness.png", floorR);
	LoadTexture(L"../../Assets/Textures/floor_metal.png", floorM);
	
	LoadTexture(L"../../Assets/Textures/paint_albedo.png", paintA);
	LoadTexture(L"../../Assets/Textures/paint_normals.png", paintN);
	LoadTexture(L"../../Assets/Textures/paint_roughness.png", paintR);
	LoadTexture(L"../../Assets/Textures/paint_metal.png", paintM);
	
	LoadTexture(L"../../Assets/Textures/scratched_albedo.png", scratchedA);
	LoadTexture(L"../../Assets/Textures/scratched_normals.png", scratchedN);
	LoadTexture(L"../../Assets/Textures/scratched_roughness.png", scratchedR);
	LoadTexture(L"../../Assets/Textures/scratched_metal.png", scratchedM);
	
	LoadTexture(L"../../Assets/Textures/bronze_albedo.png", bronzeA);
	LoadTexture(L"../../Assets/Textures/bronze_normals.png", bronzeN);
	LoadTexture(L"../../Assets/Textures/bronze_roughness.png", bronzeR);
	LoadTexture(L"../../Assets/Textures/bronze_metal.png", bronzeM);
	
	LoadTexture(L"../../Assets/Textures/rough_albedo.png", roughA);
	LoadTexture(L"../../Assets/Textures/rough_normals.png", roughN);
	LoadTexture(L"../../Assets/Textures/rough_roughness.png", roughR);
	LoadTexture(L"../../Assets/Textures/rough_metal.png", roughM);
	
	LoadTexture(L"../../Assets/Textures/wood_albedo.png", woodA);
	LoadTexture(L"../../Assets/Textures/wood_normals.png", woodN);
	LoadTexture(L"../../Assets/Textures/wood_roughness.png", woodR);
	LoadTexture(L"../../Assets/Textures/wood_metal.png", woodM);

	LoadTexture(L"../../Assets/Textures/HeronScissors.png", heronA);
	LoadTexture(L"../../Assets/Textures/Crowbar_Temp.png", wandA);

	LoadTexture(L"../../Assets/Textures/noise.png", shadowTextureSRV);
	
	LoadTexture(L"../../Assets/Textures/test/uv1.png", triFront);
	LoadTexture(L"../../Assets/Textures/test/uv1.png", triSide);
	LoadTexture(L"../../Assets/Textures/test/uv1.png", triTop);
	LoadTexture(L"../../Assets/Textures/rainbowDither.png", noise);



	// Describe and create our sampler state
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.MaxAnisotropy = 16;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, samplerOptions.GetAddressOf());


	// Create the sky using 6 images
	sky = std::make_shared<Sky>(
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\right.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\left.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\up.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\down.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\front.png").c_str(),
		FixPath(L"..\\..\\Assets\\Skies\\Clouds Blue\\back.png").c_str(),
		cubeMesh,
		skyVS,
		skyPS,
		samplerOptions,
		device,
		context);

	// Solid Mat 
	std::shared_ptr<Material> solidMat = std::make_shared<Material>( XMFLOAT3(1, 1, 1), XMFLOAT2(2, 2));


	// Create non-PBR materials
	std::shared_ptr<RendMat> cobble2xRendMat = 
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(2, 2),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(cobble2xRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(cobble2xRendMat, "Albedo", cobbleA);
	AddTextureSRV(cobble2xRendMat, "NormalMap", cobbleN);
	AddTextureSRV(cobble2xRendMat, "RoughnessMap", cobbleR);
	AddTextureSRV(cobble2xRendMat, "Lightbox", sky->GetSkySRV());


	std::shared_ptr<RendMat> paintRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(2, 2),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(paintRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(paintRendMat, "Albedo", paintA);
	AddTextureSRV(paintRendMat, "NormalMap", paintN);
	AddTextureSRV(paintRendMat, "RoughnessMap", paintR);
	AddTextureSRV(paintRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> bronzeRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(2, 2),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(bronzeRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(bronzeRendMat, "Albedo", bronzeA);
	AddTextureSRV(bronzeRendMat, "NormalMap", bronzeN);
	AddTextureSRV(bronzeRendMat, "RoughnessMap", bronzeR);
	AddTextureSRV(bronzeRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> roughRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(2, 2),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(roughRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(roughRendMat, "Albedo", roughA);
	AddTextureSRV(roughRendMat, "NormalMap", roughN);
	AddTextureSRV(roughRendMat, "RoughnessMap", roughR);
	AddTextureSRV(roughRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> woodRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(2, 2),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(woodRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(woodRendMat, "Albedo", woodA);
	AddTextureSRV(woodRendMat, "NormalMap", woodN);
	AddTextureSRV(woodRendMat, "RoughnessMap", woodR);
	AddTextureSRV(woodRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> heronRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(1, 1),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(heronRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(heronRendMat, "Albedo", heronA);
	AddTextureSRV(heronRendMat, "NormalMap", woodN);
	AddTextureSRV(heronRendMat, "RoughnessMap", woodR);
	AddTextureSRV(heronRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> wandRendMat =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(1, 1),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(wandRendMat, "BasicSampler", samplerOptions);
	AddTextureSRV(wandRendMat, "Albedo", wandA);
	AddTextureSRV(wandRendMat, "NormalMap", woodN);
	AddTextureSRV(wandRendMat, "RoughnessMap", woodR);
	AddTextureSRV(wandRendMat, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> solidCommon =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(1, 1),
			L"VertexShader.cso",
			L"PixelCommon.cso"
		);
	AddSampler(solidCommon, "BasicSampler", samplerOptions);
	AddTextureSRV(solidCommon, "Albedo", scratchedA);
	AddTextureSRV(solidCommon, "NormalMap", woodN);
	AddTextureSRV(solidCommon, "RoughnessMap", woodR);
	AddTextureSRV(solidCommon, "Lightbox", sky->GetSkySRV());

	std::shared_ptr<RendMat> triplanar =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(1, 1),
			L"VertexShader.cso",
			L"PixelTriplanar.cso"
		);
	AddSampler(triplanar,		"BasicSampler", samplerOptions);
	AddTextureSRV(triplanar,	"AlbedoFront",	woodA);
	AddTextureSRV(triplanar,	"AlbedoSide",	cobbleA);
	AddTextureSRV(triplanar,	"AlbedoTop",	scratchedA);

	std::shared_ptr<RendMat> triplanarShadows =
		std::make_shared<RendMat>(
			XMFLOAT3(1, 1, 1),
			XMFLOAT2(0, 0),
			XMFLOAT2(1, 1),
			L"VertexShader.cso",
			L"TriplanarShadows.cso"
		);
	AddSampler(triplanarShadows, "BasicSampler", samplerOptions);
	AddTextureSRV(triplanarShadows, "Albedo", triFront);
	AddTextureSRV(triplanarShadows, "AlbedoFront", noise);
	AddTextureSRV(triplanarShadows, "AlbedoSide", noise);
	AddTextureSRV(triplanarShadows, "AlbedoTop", noise);


	// Create the non-PBR entities ==============================

	// Generate Cornell-like cube 
	float yOffset = 1.0f;

	std::shared_ptr<GameEntity> leftWall = std::make_shared<GameEntity>(planeMesh, solidCommon);
	leftWall->GetTransform()->SetPosition(-2, yOffset, 0);
	leftWall->GetTransform()->Rotate(0.0f, 0.0f, -XM_PI / 2.0f);
	leftWall->GetTransform()->SetScale(2.0f);

	std::shared_ptr<GameEntity> rightWall = std::make_shared<GameEntity>(planeMesh, solidCommon);
	rightWall->GetTransform()->SetPosition(2, yOffset, 0);
	rightWall->GetTransform()->Rotate(-XM_PI / 2.0f, 0.0f, XM_PI / 2.0f);
	rightWall->GetTransform()->SetScale(2.0f);
	
	std::shared_ptr<GameEntity> backWall = std::make_shared<GameEntity>(planeMesh, solidCommon);
	backWall->GetTransform()->SetPosition(0.0f, yOffset, 2.0f);
	backWall->GetTransform()->Rotate(-XM_PI / 2.0f, 0.0f, 0.0f);
	backWall->GetTransform()->SetScale(2.0f);

	std::shared_ptr<GameEntity> floor = std::make_shared<GameEntity>(planeMesh, solidCommon);
	floor->GetTransform()->SetPosition(0.0f, -2.0f + yOffset, 0.0f);
	floor->GetTransform()->SetScale(2.0f);

	std::shared_ptr<GameEntity> roof = std::make_shared<GameEntity>(planeMesh, solidCommon);
	roof->GetTransform()->SetPosition(0.0f, 2.0f + yOffset, 0.0f);
	roof->GetTransform()->Rotate(0.0f, 0.0f, XM_PI);
	roof->GetTransform()->SetScale(2.0f);

	std::shared_ptr<GameEntity> cubeA = std::make_shared<GameEntity>(sphereMesh, solidCommon);
	cubeA->GetTransform()->SetPosition(1.0f, -1.5f + yOffset, 0.0f);
	cubeA->GetTransform()->Rotate(0.0f, XM_PI / 4.0f, 0.0f);
	cubeA->GetTransform()->SetScale(1.0f);

	std::shared_ptr<GameEntity> cubeB = std::make_shared<GameEntity>(cubeMesh, solidCommon);
	cubeB->GetTransform()->SetPosition(-1.0f, -1.25f + yOffset, 0.0f);
	cubeB->GetTransform()->Rotate(0.0f, 0.0f, 0.0f);
	cubeB->GetTransform()->SetScale(1.5f);


	swordEntity = std::make_shared<GameEntity>(planeMesh, heronRendMat, false);
	wandEntity = std::make_shared<GameEntity>(planeMesh, wandRendMat, false);

	entities.push_back(leftWall);
	entities.push_back(rightWall);
	entities.push_back(backWall);
	entities.push_back(floor);
	entities.push_back(roof);
	entities.push_back(cubeA);
	entities.push_back(cubeB);
	entities.push_back(swordEntity);
	entities.push_back(wandEntity);

	// Save assets needed for drawing point lights
	lightMesh = sphereMesh;
	lightVS = nameToVS[L"VertexShader.cso"];
	lightPS = nameToPS[L"SolidColorPS.cso"];

	GroupEntitiesByShaders();
}


// --------------------------------------------------------
// Generates the lights in the scene: 3 directional lights
// and many random point lights.
// --------------------------------------------------------
void Game::GenerateLights()
{
	// Reset
	lights.clear();

	// Setup directional lights
	Light dir1 = {};
	dir1.Type = LIGHT_TYPE_DIRECTIONAL;
	dir1.Direction = XMFLOAT3(1, -1, 1);
	dir1.Color = XMFLOAT3(0.8f, 0.8f, 0.8f);
	dir1.Intensity = 1.0f;

	Light dir2 = {};
	dir2.Type = LIGHT_TYPE_DIRECTIONAL;
	dir2.Direction = XMFLOAT3(-1, -0.25f, 0);
	dir2.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir2.Intensity = 1.0f;

	Light dir3 = {};
	dir3.Type = LIGHT_TYPE_DIRECTIONAL;
	dir3.Direction = XMFLOAT3(0, -1, 1);
	dir3.Color = XMFLOAT3(0.2f, 0.2f, 0.2f);
	dir3.Intensity = 1.0f;

	// Add light to the list
	lights.push_back(dir1);
	lights.push_back(dir2);
	lights.push_back(dir3);

	// Create the rest of the lights
	while (lights.size() < MAX_LIGHTS)
	{
		Light point = {};
		point.Type = LIGHT_TYPE_POINT;
		point.Position = XMFLOAT3(RandomRange(-10.0f, 10.0f), RandomRange(-5.0f, 5.0f), RandomRange(-10.0f, 10.0f));
		point.Color = XMFLOAT3(RandomRange(0, 1), RandomRange(0, 1), RandomRange(0, 1));
		point.Range = RandomRange(5.0f, 10.0f);
		point.Intensity = RandomRange(0.1f, 3.0f);

		// Add to the list
		lights.push_back(point);
	}

}

// --------------------------------------------------------
// Sets up the resources necessary for shadow mapping 
// --------------------------------------------------------
void Game::GenerateShadowData()
{
	// Create the actual texture that will be the shadow map
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = SHADOW_MAP_RESOLUTION; // Ideally a power of 2 (like 1024)
	shadowDesc.Height = SHADOW_MAP_RESOLUTION; // Ideally a power of 2 (like 1024)
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());


	// NOTE: We are creating two views since we are rendering
	//		 for the depth data along with creating an image. 

	// Create the depth/stencil view that lets us access the 
	// shadow texture 
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(
		shadowTexture.Get(),
		&shadowDSDesc,
		shadowDSV.GetAddressOf());

	// Create the SRV for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(
		shadowTexture.Get(),
		&srvDesc,
		shadowSRV.GetAddressOf());

	// Shadow rasterizer 
	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 1000; // Min. precision units, not world units!
	shadowRastDesc.SlopeScaledDepthBias = 1.0f; // Bias more based on slope
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	// Shadow sampler 
	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f; // Only need the first component
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);
}



// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	renderer->Resize(windowWidth, windowHeight, targetSizeX, targetSizeY);

	// Update our projection matrix to match the new aspect ratio
	for (int i = 0; i < playersData->cams.size(); i++)
	{
		UpdateProjectionMatrix(&playersData->cams[i], this->targetSizeX / (float)this->targetSizeY);
	}
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Set up the new frame for the UI, then build
	// this frame's interface.  Note that the building
	// of the UI could happen at any point during update.
	UINewFrame(deltaTime);
	BuildUI();


	// Update the player
	std::vector<PlayerInput> inputs = PlayersInputs(updateMouseDelta);
	TransformPlayers(playersData.get(), inputs, deltaTime);
	/*UpdatePlayerGameLogic(
		playersData.get(),
		swordEntity, wandEntity,
		deltaTime);*/

	// Check individual input
	Input& input = Input::GetInstance();

	static bool heldPrev = false;
	if (input.KeyDown(VK_TAB) && !heldPrev)
	{
		// Toggle mouse constatins 
		updateMouseDelta = !updateMouseDelta;
		SetCursorPos(
			GetSystemMetrics(SM_CXSCREEN) / 2, 
			GetSystemMetrics(SM_CYSCREEN) / 2);

		ShowCursor(!updateMouseDelta);

		heldPrev = true;
	}
	else if (input.KeyUp(VK_TAB))
	{
		heldPrev = false; 
	}
	if (input.KeyDown(VK_ESCAPE)) Quit();
	if (input.KeyPress(VK_TAB)) GenerateLights();
}

// --------------------------------------------------------
// Before rendering the main primary entities go through 
// and draw the shadow depths for sampling later 
// --------------------------------------------------------
void Game::DrawShadowMap()
{
	// TODO: This does not actually need to be drawn every loop.
	//		 Since our static objects do not actually move around
	//		 and the lighting also does not move then we can continue
	//		 to reuse the same shadow map 

	// Set to shadow rasterizer 
	context->RSSetState(shadowRasterizer.Get());

	// Clear shadow map 
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set up shadow depth target. We do not need
	// a color output so null for render target
	ID3D11RenderTargetView* nullRTV{};
	context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get()); 

	// Unbind pixel shader 
	context->PSSetShader(0, 0, 0);

	// Adjust viewport to match shadow map resolution 
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)SHADOW_MAP_RESOLUTION;
	viewport.Height = (float)SHADOW_MAP_RESOLUTION;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	context->RSSetViewports(1, &viewport);


	// Draw all entities to shadow map 
	std::shared_ptr<SimpleVertexShader> shadowVS = nameToVS[L"ShadowVertex.cso"];
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", shadowViewMatrix);
	shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
	// Loop and draw all entities
	for (auto& e : entities)
	{
		if (!e->castsShadows) 
			continue;

		shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();


		// Draw the mesh directly to avoid the entity's material
		// Note: Your code may differ significantly here!
		e->GetMesh()->SetBuffersAndDraw(context);
	}

	// Disable shadow rasterizer 
	context->RSSetState(0);

	// Reset pipeline
	viewport.TopLeftX = -(windowWidth - targetSizeX) / 2.0f;
	viewport.TopLeftY = -(windowHeight - targetSizeY) / 2.0f;
	viewport.Width = (float)this->windowWidth;
	viewport.Height = (float)this->windowHeight;
	context->RSSetViewports(1, &viewport);
	context->OMSetRenderTargets(
		1,
		backBufferRTV.GetAddressOf(),
		depthBufferDSV.Get());
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	renderer->DrawToTargetBuffer(backBufferRTV, depthBufferDSV, &playersData->cams[0]);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	//DrawShadowMap();
	//
	//// Frame START
	//// - These things should happen ONCE PER FRAME
	//// - At the beginning of Game::Draw() before drawing *anything*
	//{
	//	// Clear the back buffer (erases what's on the screen)
	//	const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
	//	context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);
	//
	//	// Clear the depth buffer (resets per-pixel occlusion information)
	//	context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	//}
	//
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//
	//
	//for (auto group : entityGroups)
	//{
	//	// Pixel shader is set for entire group 
	//	// since they are (currently) not entity dependent 
	//
	//	std::shared_ptr<SimplePixelShader> ps = nameToPS[group[0]->GetMaterial()->psName];
	//
	//	
	//
	//	SetPixelShader(
	//		group[0]->GetMaterial(),
	//		ps,
	//		lights[0],
	//		playersData->cams[0].transform.GetPosition(),
	//		shadowTextureSRV,
	//		shadowSRV, shadowSampler,
	//		psNameToID
	//	);
	//
	//	// TODO: Optimization is having setting of the shaders not 
	//	//		 reset repeated data. We can have a stored enum to 
	//	//		 remember the previously set shader and pass in a
	//	//		 bool that indicates whether to use set an entire
	//	//		 new shader or simply update necessary items 
	//
	//	for (auto entity : group)
	//	{
	//		// Vertex shader must be set for entire group
	//		// since they are entity dependent 
	//		SetVertexShader(
	//			nameToVS[entity->GetMaterial()->vsName],
	//			entity->GetTransform(),
	//			&playersData->cams[0],
	//			shadowViewMatrix,
	//			shadowProjectionMatrix);
	//
	//		// Draw entity 
	//		entity->Draw(context, ps);
	//	}
	//}

	//#if defined(DEBUG) || defined(_DEBUG)
	//// Debug Drawing 
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	//
	//for (auto& ge : debugDrawData.drawGroup)
	//{
	//	std::shared_ptr<SimplePixelShader> ps = nameToPS[L"SolidColorPS.cso"]; //ge.entity->GetMaterial()->GetPixelShader();
	//	ps->SetFloat3("Color", ge.color);
	//	ps->CopyBufferData("perFrame");
	//
	//	// Draw the entity
	//	ge.entity->Draw(context, ps);
	//}
	//#endif

	


	// Draw the light sources?
	//if(showPointLights)
	//	DrawPointLights();

	// Draw the sky
	//context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//sky->Draw(&playersData->cams[0]);
	//
	//// Frame END
	//// - These should happen exactly ONCE PER FRAME
	//// - At the very end of the frame (after drawing *everything*)
	//{
	//	// Draw the UI after everything else
	//	ImGui::Render();
	//	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	//
	//	// Present the back buffer to the user
	//	//  - Puts the results of what we've drawn onto the window
	//	//  - Without this, the user never sees anything
	//	bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
	//	swapChain->Present(
	//		vsyncNecessary ? 1 : 0,
	//		vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);
	//
	//	// Must re-bind buffers after presenting, as they become unbound
	//	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	//
	//	// Unbind shadowmap 
	//	ID3D11ShaderResourceView* nullSRVs[128] = {};
	//	context->PSSetShaderResources(0, 128, nullSRVs);
	//}
}


// --------------------------------------------------------
// Draws the point lights as solid color spheres
// --------------------------------------------------------
void Game::DrawPointLights()
{
	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", playersData->cams[0].viewMatrix);
	lightVS->SetMatrix4x4("projection", playersData->cams[0].projMatrix);

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[i];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		float scale = light.Range / 20.0f;

		// Make the transform for this light
		XMMATRIX rotMat = XMMatrixIdentity();
		XMMATRIX scaleMat = XMMatrixScaling(scale, scale, scale);
		XMMATRIX transMat = XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		XMMATRIX worldMat = scaleMat * rotMat * transMat;

		XMFLOAT4X4 world;
		XMFLOAT4X4 worldInvTrans;
		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}

}

/// <summary>
/// Regenerate the light view matrix and other directional
/// light data   
/// </summary>
void Game::OnWorldLightChange()
{
	XMFLOAT3 mainLightDir = lights[0].Direction;
	XMVECTOR loadedDir = XMLoadFloat3(&mainLightDir);

	XMMATRIX lightView = XMMatrixLookToLH(
		-loadedDir * 20, // Position: "Backing up" 20 units from origin
		loadedDir, // Direction: light's direction
		XMVectorSet(0, 1, 0, 0)); // Up: World up vector (Y axis)

	float lightProjectionSize = 15.0f; // Tweak for your scene!
	XMMATRIX lightProjection = XMMatrixOrthographicLH(
		lightProjectionSize,
		lightProjectionSize,
		1.0f,
		100.0f);

	XMStoreFloat4x4(&shadowViewMatrix, lightView);
	XMStoreFloat4x4(&shadowProjectionMatrix, lightProjection);
}

/// <summary>
/// Distribute group of active entities into shader groups. 
/// Used for reseting all entities or loading a scene 
/// </summary>
void Game::GroupEntitiesByShaders()
{
	// Cleanup 
	entityGroups.clear();


	std::unordered_map<std::shared_ptr<SimplePixelShader>, int> psToIndex;
	for (auto entity : entities)
	{
		// Current entity shaders 
		std::shared_ptr<SimplePixelShader> ps =
			nameToPS[entity->GetMaterial()->psName];
		std::shared_ptr<SimpleVertexShader> vs =
			nameToVS[entity->GetMaterial()->vsName];

		// Check if pixel shader exists 
		if (psToIndex.find(ps) == psToIndex.end())
		{
			// Remember new ps index relation 
			psToIndex.insert(
				std::pair<std::shared_ptr<SimplePixelShader>, int>
				(ps, entityGroups.size()));

			// Add new entity group which all use this type of ps 
			entityGroups.push_back(std::vector<std::shared_ptr<GameEntity>>());
		}
		
		entityGroups[psToIndex[ps]].push_back(entity);
	}
}

// --------------------------------------------------------
// Prepares a new frame for the UI, feeding it fresh
// input and time information for this new frame.
// --------------------------------------------------------
void Game::UINewFrame(float deltaTime)
{
	// Get a reference to our custom input manager
	Input& input = Input::GetInstance();

	// Reset input manager's gui state so we don’t
	// taint our own input
	input.SetKeyboardCapture(false);
	input.SetMouseCapture(false);

	// Feed fresh input data to ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)this->windowWidth;
	io.DisplaySize.y = (float)this->windowHeight;

	// Reset the frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Determine new input capture
	input.SetKeyboardCapture(io.WantCaptureKeyboard);
	input.SetMouseCapture(io.WantCaptureMouse);
}


// --------------------------------------------------------
// Builds the UI for the current frame
// --------------------------------------------------------
void Game::BuildUI()
{

	// Should we show the built-in demo window?
	if (showUIDemoWindow)
	{
		ImGui::ShowDemoWindow();
	}

	// Shadow map GUI
	ImGui::Begin("Shadow map");
	{
		ImGui::Image(shadowSRV.Get(), ImVec2(512, 512));
	}
	ImGui::End();

	// Actually build our custom UI, starting with a window
	ImGui::Begin("Inspector");
	{
		// Set a specific amount of space for widget labels
		ImGui::PushItemWidth(-160); // Negative value sets label width

		// === Overall details ===
		if (ImGui::TreeNode("App Details"))
		{
			ImGui::Spacing();
			ImGui::Text("Frame rate: %f fps", ImGui::GetIO().Framerate);
			ImGui::Text("Window Client Size: %dx%d", windowWidth, windowHeight);

			ImGui::Spacing();
			ImGui::Text("Scene Details");
			ImGui::Text("Top Row:");    ImGui::SameLine(125); ImGui::Text("PBR Materials");
			ImGui::Text("Bottom Row:"); ImGui::SameLine(125); ImGui::Text("Non-PBR Materials");
	
			// Should we show the demo window?
			ImGui::Spacing();
			if (ImGui::Button(showUIDemoWindow ? "Hide ImGui Demo Window" : "Show ImGui Demo Window"))
				showUIDemoWindow = !showUIDemoWindow;

			ImGui::Spacing();

			// Finalize the tree node
			ImGui::TreePop();
		}
		
		// === Controls ===
		if (ImGui::TreeNode("Controls"))
		{
			ImGui::Spacing();
			ImGui::Text("(WASD, X, Space)");    ImGui::SameLine(175); ImGui::Text("Move camera");
			ImGui::Text("(Left Click & Drag)"); ImGui::SameLine(175); ImGui::Text("Rotate camera");
			ImGui::Text("(Left Shift)");        ImGui::SameLine(175); ImGui::Text("Hold to speed up camera");
			ImGui::Text("(Left Ctrl)");         ImGui::SameLine(175); ImGui::Text("Hold to slow down camera");
			ImGui::Text("(TAB)");               ImGui::SameLine(175); ImGui::Text("Randomize lights");
			ImGui::Spacing();

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Camera details ===
		if (ImGui::TreeNode("Camera"))
		{
			// Show UI for current camera
			//CameraUI(camera);

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Entities ===
		if (ImGui::TreeNode("Scene Entities"))
		{
			// Loop and show the details for each entity
			for (int i = 0; i < entities.size(); i++)
			{
				// New node for each entity
				// Note the use of PushID(), so that each tree node and its widgets
				// have unique internal IDs in the ImGui system
				ImGui::PushID(i);
				if (ImGui::TreeNode("Entity Node", "Entity %d", i))
				{
					// Build UI for one entity at a time
					EntityUI(entities[i]);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// Finalize the tree node
			ImGui::TreePop();
		}

		// === Lights ===
		if (ImGui::TreeNode("Lights"))
		{
			// Light details
			ImGui::Spacing();
			ImGui::SliderInt("Light Count", &lightCount, 0, MAX_LIGHTS);
			ImGui::Checkbox("Show Point Lights", &showPointLights);
			ImGui::Spacing();

			// Loop and show the details for each entity
			for (int i = 0; i < lightCount; i++)
			{
				// Name of this light based on type
				std::string lightName = "Light %d";
				switch (lights[i].Type)
				{
				case LIGHT_TYPE_DIRECTIONAL: lightName += " (Directional)"; break;
				case LIGHT_TYPE_POINT: lightName += " (Point)"; break;
				case LIGHT_TYPE_SPOT: lightName += " (Spot)"; break;
				}

				// New node for each light
				// Note the use of PushID(), so that each tree node and its widgets
				// have unique internal IDs in the ImGui system
				ImGui::PushID(i);
				if (ImGui::TreeNode("Light Node", lightName.c_str(), i))
				{
					// Build UI for one entity at a time
					LightUI(lights[i]);

					ImGui::TreePop();
				}
				ImGui::PopID();
			}

			// Finalize the tree node
			ImGui::TreePop();
		}
	}
	ImGui::End();
}


// --------------------------------------------------------
// Builds the UI for a single camera
// --------------------------------------------------------
void Game::CameraUI(std::shared_ptr<FreeCamera> cam)
{
	ImGui::Spacing();

	// Transform details
	XMFLOAT3 pos = cam->GetTransform()->GetPosition();
	XMFLOAT3 rot = cam->GetTransform()->GetPitchYawRoll();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
		cam->GetTransform()->SetPosition(pos);
	if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f))
		cam->GetTransform()->SetRotation(rot);
	ImGui::Spacing();

	// Clip planes
	float nearClip = cam->GetNearClip();
	float farClip = cam->GetFarClip();
	if (ImGui::DragFloat("Near Clip Distance", &nearClip, 0.01f, 0.001f, 1.0f))
		cam->SetNearClip(nearClip);
	if (ImGui::DragFloat("Far Clip Distance", &farClip, 1.0f, 10.0f, 1000.0f))
		cam->SetFarClip(farClip);

	// Projection type
	CameraProjectionType projType = cam->GetProjectionType();
	int typeIndex = (int)projType;
	if (ImGui::Combo("Projection Type", &typeIndex, "Perspective\0Orthographic"))
	{
		projType = (CameraProjectionType)typeIndex;
		cam->SetProjectionType(projType);
	}

	// Projection details
	if (projType == CameraProjectionType::Perspective)
	{
		// Convert field of view to degrees for UI
		float fov = cam->GetFieldOfView() * 180.0f / XM_PI;
		if (ImGui::SliderFloat("Field of View (Degrees)", &fov, 0.01f, 180.0f))
			cam->SetFieldOfView(fov * XM_PI / 180.0f); // Back to radians
	}
	else if (projType == CameraProjectionType::Orthographic)
	{
		float wid = cam->GetOrthographicWidth();
		if (ImGui::SliderFloat("Orthographic Width", &wid, 1.0f, 10.0f))
			cam->SetOrthographicWidth(wid);
	}

	ImGui::Spacing();
}


// --------------------------------------------------------
// Builds the UI for a single entity
// --------------------------------------------------------
void Game::EntityUI(std::shared_ptr<GameEntity> entity)
{
	ImGui::Spacing();

	// Transform details
	Transform* trans = entity->GetTransform();
	XMFLOAT3 pos = trans->GetPosition();
	XMFLOAT3 rot = trans->GetPitchYawRoll();
	XMFLOAT3 sca = trans->GetScale();

	if (ImGui::DragFloat3("Position", &pos.x, 0.01f)) trans->SetPosition(pos);
	if (ImGui::DragFloat3("Rotation (Radians)", &rot.x, 0.01f)) trans->SetRotation(rot);
	if (ImGui::DragFloat3("Scale", &sca.x, 0.01f)) trans->SetScale(sca);

	// Mesh details
	ImGui::Spacing();
	ImGui::Text("Mesh Index Count: %d", entity->GetMesh()->GetIndexCount());

	ImGui::Spacing();
}


// --------------------------------------------------------
// Builds the UI for a single light
// --------------------------------------------------------
void Game::LightUI(Light& light)
{
	// Light type
	if (ImGui::RadioButton("Directional", light.Type == LIGHT_TYPE_DIRECTIONAL))
	{
		light.Type = LIGHT_TYPE_DIRECTIONAL;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Point", light.Type == LIGHT_TYPE_POINT))
	{
		light.Type = LIGHT_TYPE_POINT;
	}
	ImGui::SameLine();

	if (ImGui::RadioButton("Spot", light.Type == LIGHT_TYPE_SPOT))
	{
		light.Type = LIGHT_TYPE_SPOT;
	}

	// Direction
	if (light.Type == LIGHT_TYPE_DIRECTIONAL || light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::DragFloat3("Direction", &light.Direction.x, 0.1f);

		// Normalize the direction
		XMVECTOR dirNorm = XMVector3Normalize(XMLoadFloat3(&light.Direction));
		XMStoreFloat3(&light.Direction, dirNorm);
	}

	// Position & Range
	if (light.Type == LIGHT_TYPE_POINT || light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::DragFloat3("Position", &light.Position.x, 0.1f);
		ImGui::SliderFloat("Range", &light.Range, 0.1f, 100.0f);
	}

	// Spot falloff
	if (light.Type == LIGHT_TYPE_SPOT)
	{
		ImGui::SliderFloat("Spot Falloff", &light.SpotFalloff, 0.1f, 128.0f);
	}

	// Color details
	ImGui::ColorEdit3("Color", &light.Color.x);
	ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 10.0f);
}


