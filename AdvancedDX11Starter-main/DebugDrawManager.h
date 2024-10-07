#pragma once

#include <DirectXMath.h>
#include <vector>

#include "Transform.h"
#include "string"

#include "Mesh.h"

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

struct DebugDrawData
{
	/// <summary>
	/// Initiate the debug manager 
	/// </summary>
	//DebugDrawData(Microsoft::WRL::ComPtr<ID3D11Device> device)
	//{
	//	// Import the debug meshes 
	//}

	std::vector<Mesh> drawGroup; 
	std::vector<float> lifeTimes; 

private:
	Mesh debugMeshes[9];
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
static void AddLine(
	DirectX::XMFLOAT3 pointA,
	DirectX::XMFLOAT3 pointB,
	DirectX::XMFLOAT4 color,
	float lineWidth = 1.0f,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds na axis-aligned cross (3 lines converging at a point)
/// to the debug drawing queue  
/// </summary>
static void AddCross(
	DirectX::XMFLOAT3 point,
	DirectX::XMFLOAT4 color,
	float size,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a wireframe sphere to the debug drawing queue 
/// </summary>
static void AddSphere(
	DirectX::XMFLOAT3 point,
	float radius,
	DirectX::XMFLOAT4 color,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a circle to the debug drawing queue 
/// </summary>
static void AddCircle(
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
static void AddAxes(
	Transform& xfm,
	DirectX::XMFLOAT4 color,
	float size,
	float duration = 0.0f)
{

}

/// <summary>
/// Adds a wireframe triangle to the debug drawing queue 
/// </summary>
static void AddTriangle(
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
static void AddAABB(
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
static void AddOBB(
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
static void AddString(
	DirectX::XMFLOAT3 pos,
	std::string text,
	DirectX::XMFLOAT4 color,
	float duration = 0.0f)
{

}

#pragma endregion