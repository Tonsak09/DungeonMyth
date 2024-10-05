#pragma once
#include <DirectXMath.h>
#include <vector>

#include "Transform.h"
#include "Camera.h"

#include "Input.h"

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
	std::vector<Camera> cams;
	std::vector<float> camHeight;
	std::vector<float> moveSpeed;
	std::vector<float> turnSpeed; 
};

/// <summary>
/// Represents a single pass of player input 
/// </summary>
struct PlayerInput
{
	DirectX::XMFLOAT3 dir;
	DirectX::XMFLOAT2 mouseDelta; 
};

/// <summary>
/// Add a player to the data struct 
/// </summary>
/// <param name="data"></param>
/// <param name="name"></param>
static void AddPlayer(PlayersData* data, std::string id, float camRatio)
{
	// Assign name to index 

	// Generate camera 
	Camera camera = Camera();
	InitCamera(&camera,
		DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f),	// Position
		DirectX::XM_PIDIV4,			// Field of view
		camRatio,  // Aspect ratio
		0.01f,				// Near clip
		100.0f,				// Far clip
		CameraProjectionType::Perspective);

	data->cams.push_back(camera);
	data->transforms.push_back(Transform());
	data->camHeight.push_back(0.0f);
	data->moveSpeed.push_back(5.0f);
	data->turnSpeed.push_back(1.0f);
}

/// <summary>
/// Moves all player entities based on directional inputs 
/// </summary>
/// <param name="data"></param>
/// <param name="dirs"></param>
static void TransformPlayers(PlayersData* data, std::vector<PlayerInput> inputs, float delta)
{

	// TODO: Split into multiple for loops to avoid cache misses 

	for (int i = 0; i < data->transforms.size(); i++)
	{
		// Translate base position 
		DirectX::XMFLOAT3 dir = inputs[i].dir;

		float s = data->moveSpeed[i] * delta;
		DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(s, s, s);

		DirectX::XMFLOAT3 baseFinal;
		DirectX::XMVECTOR diff = DirectX::XMVectorMultiply(DirectX::XMLoadFloat3(&dir), DirectX::XMLoadFloat3(&scale));
		DirectX::XMStoreFloat3(&baseFinal, diff);

		data->transforms[i].MoveRelative(baseFinal);

		DirectX::XMFLOAT3 pos = data->transforms[i].GetPosition();


		// Camera position offset 
		DirectX::XMFLOAT3 camPos; 
		DirectX::XMFLOAT3 up = data->transforms[i].GetUp();
		DirectX::XMStoreFloat3(&camPos, 
			DirectX::XMVectorAdd(
				DirectX::XMLoadFloat3(&pos), 
				DirectX::XMLoadFloat3(&up)));
		data->cams[i].transform.SetPosition(camPos);



		// Rotation 
		DirectX::XMFLOAT2 mouseDelta = inputs[i].mouseDelta;

		// Roate base AND cam for x-delta 
		data->transforms[i].Rotate(
			0.0f,
			data->turnSpeed[i] * mouseDelta.x * delta, 0.0f);
		data->cams[i].transform.Rotate(
			data->turnSpeed[i] * mouseDelta.y * delta,
			data->turnSpeed[i] * mouseDelta.x * delta, 0.0f);

		//printf("%f, %f\n", mouseDelta.x, mouseDelta.y);


		UpdateViewMatrix(&data->cams[i]);
	}
}



/// <summary>
/// Gets all the current input data of players and
/// organizes it into a sweet little vector 
/// </summary>
/// <param name="delta"></param>
static std::vector<PlayerInput> PlayersInputs(bool updateMouseDelta)
{
	// In its current state only worry about one player's 
	// inputs 

	Input& input = Input::GetInstance(); 

	std::vector<PlayerInput> inputs;

	{ // Represents a single player 

		PlayerInput curr = PlayerInput();  


		


		// Directional 
		DirectX::XMFLOAT3 dirInput;
		dirInput = DirectX::XMFLOAT3(
			-input.KeyDown('A') + input.KeyDown('D'),
			0.0f,
			-input.KeyDown('S') + input.KeyDown('W'));

		DirectX::XMStoreFloat3(&dirInput,
			DirectX::XMVector3Normalize(XMLoadFloat3(&dirInput)));


		// Rotational 
		if (updateMouseDelta)
		{
			float xMid = 640;
			float yMid = 360;

			// Mouse Data
			POINT mousePosPoint = {};
			GetCursorPos(&mousePosPoint);

			static DirectX::XMFLOAT2 center = DirectX::XMFLOAT2(xMid, yMid);
			static DirectX::XMFLOAT2 mousePos;

			mousePos = DirectX::XMFLOAT2(
				(float)mousePosPoint.x,
				(float)mousePosPoint.y
			);


			DirectX::XMFLOAT2 delta;
			DirectX::XMStoreFloat2(&delta, DirectX::XMVectorSubtract(
				XMLoadFloat2(&mousePos), XMLoadFloat2(&center)));

			float xDiff = abs(delta.x) >= 100.0 ? 0.0 : delta.x;
			float yDiff = abs(delta.y) >= 100.0 ? 0.0 : delta.y;
			DirectX::XMFLOAT2 mouseDelta(xDiff, yDiff);

			SetCursorPos(xMid, yMid);
			curr.mouseDelta = mouseDelta;
		}
		else
		{
			curr.mouseDelta = DirectX::XMFLOAT2(0.0f, 0.0f);
		}
		

		curr.dir = dirInput;
		inputs.push_back(curr);
	}


	return inputs;
}