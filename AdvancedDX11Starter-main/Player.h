#pragma once
#include <DirectXMath.h>
#include <vector>

#include "Transform.h"
#include "Camera.h"

/// <summary>
/// Holds data relating to all players on the server 
/// </summary>
struct PlayersData
{
public:
	// Player Abilities 

	// Player Gameplay Resources 

	// Player Inventory Bag

	// Player Inventory Hands 

	// Player Transform Data
	std::vector<Transform> transforms;
	std::vector<FreeCamera> cams;
	std::vector<float> camHeight;
	std::vector<float> moveSpeed;
	std::vector<float> turnSpeed; 
};

/// <summary>
/// Moves all player entities based on directional inputs 
/// </summary>
/// <param name="data"></param>
/// <param name="dirs"></param>
static void MovePlayers(PlayersData* data, std::vector<DirectX::XMFLOAT3> dirs, float delta)
{
	for (int i = 0; i < data->transforms.size(); i++)
	{
		DirectX::XMFLOAT3 dir = dirs[i];
		float s = data->moveSpeed[i] * delta;
		DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(s, s, s);

		DirectX::XMFLOAT3 final;
		DirectX::XMVECTOR diff = DirectX::XMVectorMultiply(DirectX::XMLoadFloat3(&dir), DirectX::XMLoadFloat3(&scale));
		DirectX::XMStoreFloat3(&final, diff);

		data->transforms[i].MoveRelative(final);
	}
}

/// <summary>
/// Add a player to the data struct 
/// </summary>
/// <param name="data"></param>
/// <param name="name"></param>
static void AddPlayer(PlayersData* data, std::string name)
{
	// Assign name to index 

	data->transforms.push_back(Transform());
	data->camHeight.push_back(0.0f);
	data->moveSpeed.push_back(1.0f);
	data->turnSpeed.push_back(1.0f);
}