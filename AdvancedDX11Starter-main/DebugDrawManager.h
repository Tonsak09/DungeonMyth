#pragma once

#include <DirectXMath.h>
#include <vector>

#include "Transform.h"
#include "string"

#include "Mesh.h"
#include "Helpers.h"

// Helper macros for making texture and shader loading code more succinct
#define LoadTexture(file, srv) CreateWICTextureFromFile(device.Get(), context.Get(), FixPath(file).c_str(), 0, srv.GetAddressOf())
#define LoadShader(type, file) std::make_shared<type>(device.Get(), context.Get(), FixPath(file).c_str())

enum DebugMeshType
{
	LINE		= 0,
	CROSS		= 1,
	SPHERE		= 2,
	CIRCLE		= 3,
	AXES		= 4,
	TRIANGLE	= 5,
	AABB		= 6,
	OBB			= 7,
	STRING		= 8
};

struct DebugEntity
{
	std::shared_ptr<GameEntity> entity;
	DirectX::XMFLOAT3 color;

	DebugEntity() 
	{
		entity = nullptr; 
		color = DirectX::XMFLOAT3(0, 0, 0);
	}
};

struct DebugDrawData
{
	std::vector<DebugEntity> drawGroup;
	std::vector<std::shared_ptr<Mesh>> debugMeshes;
	std::shared_ptr<Material> debugMat;

	std::vector<float> lifeTimes;

	DebugDrawData() {}

	/// <summary>
	/// Initiate the debug manager 
	/// </summary>
	DebugDrawData(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		Microsoft::WRL::ComPtr<ID3D11Device> device)
	{
		// Load Shaders 
		std::shared_ptr<SimpleVertexShader> vertexShader = 
			LoadShader(SimpleVertexShader, L"VertexShader.cso");
		std::shared_ptr<SimplePixelShader> solidColorPS =
			LoadShader(SimplePixelShader, L"SolidColorPS.cso");

		std::shared_ptr<Mesh> sphereMesh = std::make_shared<Mesh>(
			FixPath(L"../../Assets/Models/sphere.obj").c_str(), device);

		debugMat = std::make_shared<Material>(
			solidColorPS, vertexShader, DirectX::XMFLOAT3(1, 1, 1), DirectX::XMFLOAT2(2, 2));
		debugMeshes.push_back(sphereMesh);
		debugMeshes.push_back(sphereMesh);
		debugMeshes.push_back(sphereMesh);

		drawGroup = std::vector<DebugEntity>();
	}
};

/// <summary>
/// Update the lifetimes of the debug draw items and remove
/// if lifetime has reached <= 0
/// </summary>
static void ManageDebugLife(DebugDrawData* DDD, float delta)
{
	for (int i = 0; i < DDD->lifeTimes.size(); i++)
	{
		DDD->lifeTimes[i] -= delta;

		// Remove from both arrays if lifetime is complete 
		if (DDD->lifeTimes[i] <= 0.0f)
		{
			DDD->lifeTimes.erase(DDD->lifeTimes.begin() + i);
			DDD->drawGroup.erase(DDD->drawGroup.begin() + i);
			i--;
		}
	}
}

#pragma region AddToDrawGroup

/// <summary>
/// Adds a line segement to debug drawing queue 
/// </summary>
static void AddDebugLine(
	DebugDrawData* DDD,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	DirectX::XMFLOAT3 pointA,
	DirectX::XMFLOAT3 pointB,
	DirectX::XMFLOAT3 color,
	float lineWidth = 1.0f,
	float duration = 0.0f)
{
	// Generate a mesh that simply has its verticies at 
	// pointA and pointB 

	Vertex vertA = {};
	Vertex vertB = {};
	vertA.Position = DirectX::XMFLOAT3(pointA);
	vertB.Position = DirectX::XMFLOAT3(pointB);

	Vertex* vertArr = new Vertex[3];
	vertArr[0] = vertA;
	vertArr[1] = vertB;
	vertArr[2] = vertB;

	unsigned int indexArr[] = { 0, 1, 1 };
	std::shared_ptr<Mesh> lineMesh = std::make_shared<Mesh>(vertArr, 3, indexArr, 3, device);

	std::shared_ptr<GameEntity> sphere = std::make_shared<GameEntity>(
		lineMesh, DDD->debugMat);
	sphere->GetTransform()->SetPosition(pointA);

	DebugEntity de;
	de.entity = sphere;
	de.color = color;
	DDD->drawGroup.push_back(de);
}

/// <summary>
/// Adds na axis-aligned cross (3 lines converging at a point)
/// to the debug drawing queue  
/// </summary>
static void AddDebugCross(
	DirectX::XMFLOAT3 point,
	DirectX::XMFLOAT4 color,
	float size,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a wireframe sphere to the debug drawing queue 
/// </summary>
static void AddDebugSphere(
	DebugDrawData* DDD,
	DirectX::XMFLOAT3 point,
	float radius,
	DirectX::XMFLOAT3 color,
	float duration = 0.0f)
{
	std::shared_ptr<GameEntity> sphere = std::make_shared<GameEntity>(
		DDD->debugMeshes[SPHERE], DDD->debugMat);
	sphere->GetTransform()->SetPosition(point);
	sphere->GetTransform()->SetScale(radius);

	DebugEntity de;
	de.entity = sphere;
	de.color = color;
	DDD->drawGroup.push_back(de);
}

/// <summary>
/// Adds a circle to the debug drawing queue 
/// </summary>
static void AddDebugCircle(
	DirectX::XMFLOAT3 point,
	DirectX::XMFLOAT3 normal,
	float radius,
	DirectX::XMFLOAT4 color,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a set of coordinate axes depicting the 
/// position and orientation ofthe given
/// transformation to the debug drawing queue 
/// </summary>
static void AddDebugAxes(
	Transform& xfm,
	DirectX::XMFLOAT4 color,
	float size,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a wireframe triangle to the debug drawing queue 
/// </summary>
static void AddDebugTriangle(
	DirectX::XMFLOAT3 vertA,
	DirectX::XMFLOAT3 vertB,
	DirectX::XMFLOAT3 vertC,
	DirectX::XMFLOAT4 color,
	float lineWidth = 1.0f,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds an axis-alligned bounding box to the debug
/// queue 
/// </summary>
/// <param name="minCoords"></param>
/// <param name="maxCoords"></param>
/// <param name="color"></param>
/// <param name="lineWidth"></param>
/// <param name="duration"></param>
static void AddDebugAABB(
	DirectX::XMFLOAT3 minCoords,
	DirectX::XMFLOAT3 maxCoords,
	DirectX::XMFLOAT4 color,
	float lineWidth = 1.0f,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds an oriented bounding box to the debug queue 
/// </summary>
static void AddDebugOBB(
	DirectX::XMFLOAT4X4 centerTransform,
	DirectX::XMFLOAT3 scaleXYZ,
	DirectX::XMFLOAT4 color,
	float lineWidth = 1.0f,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a text string to the debug drawing queue 
/// </summary>
static void AddDebugString(
	DirectX::XMFLOAT3 pos,
	std::string text,
	DirectX::XMFLOAT4 color,
	float duration = 0.0f)
{

}

#pragma endregion