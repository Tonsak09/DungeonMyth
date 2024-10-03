#pragma once
#include <DirectXMath.h>

#include "Transform.h"

enum class CameraProjectionType
{
	Perspective,
	Orthographic
};

class FreeCamera
{
public:
	FreeCamera(
		DirectX::XMFLOAT3 position, 
		float moveSpeed, 
		float mouseLookSpeed, 
		float fieldOfView, 
		float aspectRatio, 
		float nearClip = 0.01f, 
		float farClip = 100.0f, 
		CameraProjectionType projType = CameraProjectionType::Perspective);

	FreeCamera(
		float x, float y, float z, 
		float moveSpeed, 
		float mouseLookSpeed, 
		float fieldOfView,
		float aspectRatio, 
		float nearClip = 0.01f, 
		float farClip = 100.0f, 
		CameraProjectionType projType = CameraProjectionType::Perspective);

	~FreeCamera();

	// Updating methods
	void Update(float dt);
	void UpdateViewMatrix();
	void UpdateProjectionMatrix(float aspectRatio);

	// Getters
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();
	Transform* GetTransform();
	float GetAspectRatio();

	float GetFieldOfView();
	void SetFieldOfView(float fov);

	float GetMovementSpeed();
	void SetMovementSpeed(float speed);

	float GetMouseLookSpeed();
	void SetMouseLookSpeed(float speed);

	float GetNearClip();
	void SetNearClip(float distance);

	float GetFarClip();
	void SetFarClip(float distance);

	float GetOrthographicWidth();
	void SetOrthographicWidth(float width);

	CameraProjectionType GetProjectionType();
	void SetProjectionType(CameraProjectionType type);

private:
	// Camera matrices
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	Transform transform;

	float movementSpeed;
	float mouseLookSpeed;

	float fieldOfView;
	float aspectRatio;
	float nearClip;
	float farClip;
	float orthographicWidth;

	CameraProjectionType projectionType;
};

/// <summary>
/// Camera Data 
/// </summary>
struct Camera
{

public:

	// Camera matrices
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	CameraProjectionType projectionType;

	Transform transform;

	float fieldOfView;
	float aspectRatio;
	float nearClip;
	float farClip;
	float orthographicWidth;
};

static void UpdateViewMatrix(Camera* camera)
{
	// Get the camera's forward vector and position
	DirectX::XMFLOAT3 forward = camera->transform.GetForward();
	DirectX::XMFLOAT3 pos = camera->transform.GetPosition();

	// Make the view matrix and save
	DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(
		DirectX::XMLoadFloat3(&pos),
		DirectX::XMLoadFloat3(&forward),
		DirectX::XMVectorSet(0, 1, 0, 0)); // World up axis
	DirectX::XMStoreFloat4x4(&camera->viewMatrix, view);
}

static void SetCamPos(Camera* camera, DirectX::XMFLOAT3 pos)
{
	camera->transform.SetPosition(pos);
}

static void UpdateProjectionMatrix(Camera* camera, float aspectRatio)
{
	camera->aspectRatio = aspectRatio;

	DirectX::XMMATRIX P;

	// Which type?
	if (camera->projectionType == CameraProjectionType::Perspective)
	{
		P = DirectX::XMMatrixPerspectiveFovLH(
			camera->fieldOfView,		// Field of View Angle
			camera->aspectRatio,		// Aspect ratio
			camera->nearClip,			// Near clip plane distance
			camera->farClip);			// Far clip plane distance
	}
	else // CameraProjectionType::ORTHOGRAPHIC
	{
		P = DirectX::XMMatrixOrthographicLH(
			camera->orthographicWidth,	// Projection width (in world units)
			camera->orthographicWidth / aspectRatio,// Projection height (in world units)
			camera->nearClip,			// Near clip plane distance 
			camera->farClip);			// Far clip plane distance
	}

	XMStoreFloat4x4(&camera->projMatrix, P);
}

static void InitCamera(Camera* camera, 
	DirectX::XMFLOAT3 position,
	float fieldOfView,
	float aspectRatio,
	float nearClip,
	float farClip,
	CameraProjectionType projType)
{
	camera->transform.SetPosition(position);
	camera->fieldOfView = fieldOfView;
	camera->aspectRatio = aspectRatio;
	camera->nearClip = nearClip;
	camera->farClip = farClip;
	camera->projectionType = projType;

	camera->transform.SetPosition(position);

	UpdateViewMatrix(camera);
	UpdateProjectionMatrix(camera, aspectRatio);
}