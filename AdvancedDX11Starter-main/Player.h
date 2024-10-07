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
	std::vector<float> playerAcls;		// Acceleration 
	std::vector<float> playerDcls;		// Decceleration 
	std::vector<DirectX::XMFLOAT3> playerVels;		// Velocity  
	std::vector<float> playerMaxSpeed;	// Max Speed  
	std::vector<float> mouseSensitivity; // Necessary? 
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
	data->playerAcls.push_back(15.0f);
	data->playerDcls.push_back(30.0f);
	data->playerVels.push_back(DirectX::XMFLOAT3(0, 0, 0));
	data->playerMaxSpeed.push_back(8.0f);
	data->mouseSensitivity.push_back(0.5f);
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
		DirectX::XMFLOAT3 dir = inputs[i].dir; // WASD input 

		// New physical movement 
		DirectX::XMFLOAT3 cVel = data->playerVels[i];
		float cAcl = data->playerAcls[i]; // Player's accel 
		DirectX::XMVECTOR vel = DirectX::XMLoadFloat3( 
			&cVel); // Current vel 

		float acl = delta * cAcl;
		DirectX::XMVECTOR deltaV;
		deltaV = DirectX::XMVectorScale( // Change in vel 
			DirectX::XMLoadFloat3(&dir),
			acl);
		
		// Clamp new velocity 
		vel = DirectX::XMVectorAdd(vel, deltaV); // Combine 
		DirectX::XMFLOAT3 combined; 
		DirectX::XMStoreFloat3(&combined, vel);

		// Make sure there is some input 
		if (abs(dir.x) >= 0.1f || abs(dir.z) >= 0.1f)
		{
			DirectX::XMVECTOR vLength = 
				DirectX::XMVector3Length(vel);
			DirectX::XMFLOAT3 speed; // Magnitude of vel 
			DirectX::XMStoreFloat3(&speed, vLength);

			//vel = DirectX::XMVector3ClampLength( // Clamp 
			//	vel,
			//	-1.0f,
			//	1.0);

			// Check if speed is out of range 
			if (abs(speed.x) >= data->playerMaxSpeed[i])
			{
				vel = DirectX::XMVectorScale(
					DirectX::XMVector3Normalize(vel),
					data->playerMaxSpeed[i]);
			}

			


			// Store new velocity 
			DirectX::XMStoreFloat3(&cVel, vel);

			// Stop movement in direction if intent is opposite 
			if (dir.x * cVel.x < 0.0)
			{
				cVel.x = 0.0f;
			}

			if (dir.z * cVel.z < 0.0)
			{
				cVel.z = 0.0f;
			}

		}
		else
		{
			float dacl = data->playerDcls[i] * delta;

			data->playerVels[i] = DirectX::XMFLOAT3(
				cVel.x = (cVel.x < 0 ? 
					min(0, cVel.x + dacl) :
					max(0, cVel.x - dacl)),
				cVel.y, 
				cVel.z = (cVel.z < 0 ?
					min(0, cVel.z + dacl) :
					max(0, cVel.z - dacl)));
		}

		// Use displacement 
		DirectX::XMFLOAT3 d; 
		DirectX::XMVECTOR displacement = DirectX::XMVectorScale( 
			vel, 
			delta);
		DirectX::XMStoreFloat3(&d, displacement);
		data->transforms[i].MoveRelative(d);
		data->playerVels[i] = cVel;



		// Camera position offset 
		DirectX::XMFLOAT3 pos = data->transforms[i].GetPosition();
		DirectX::XMFLOAT3 camPos; 
		DirectX::XMFLOAT3 up = data->transforms[i].GetUp();
		DirectX::XMStoreFloat3(&camPos, 
			DirectX::XMVectorAdd(
				DirectX::XMLoadFloat3(&pos), 
				DirectX::XMLoadFloat3(&up)));
		data->cams[i].transform.SetPosition(camPos);



		// Rotation 
		DirectX::XMFLOAT2 mouseDelta = inputs[i].mouseDelta;

		float sensitivity = data->mouseSensitivity[i];
		float xRot = data->mouseSensitivity[i] * mouseDelta.x * sensitivity * delta;


		// Roate base AND cam for x-delta 
		data->transforms[i].Rotate(
			0.0f,
			xRot, 0.0f);
		data->cams[i].transform.Rotate(
			data->mouseSensitivity[i] * mouseDelta.y * sensitivity * delta,
			xRot, 0.0f);


		// Clamp the X rotation
		DirectX::XMFLOAT3 rot = data->cams[i].transform.GetPitchYawRoll();
		rot.x = max(-1.2f, min(rot.x, 1.2f));
		data->cams[i].transform.SetRotation(rot);

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
			static int xMid = GetSystemMetrics(SM_CXSCREEN) / 2;
			static int yMid = GetSystemMetrics(SM_CYSCREEN) / 2;

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

			DirectX::XMFLOAT2 mouseDelta(delta.x, delta.y);

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