#include "Camera.h"
#include "Input.h"

using namespace DirectX;


FreeCamera::FreeCamera(
	float x, 
	float y, 
	float z, 
	float moveSpeed, 
	float mouseLookSpeed, 
	float fieldOfView, 
	float aspectRatio, 
	float nearClip,
	float farClip,
	CameraProjectionType projType) 
	:
	movementSpeed(moveSpeed),
	mouseLookSpeed(mouseLookSpeed),
	fieldOfView(fieldOfView),
	aspectRatio(aspectRatio),
	nearClip(nearClip),
	farClip(farClip),
	projectionType(projType),
	orthographicWidth(2.0f)
{
	transform.SetPosition(x, y, z);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

FreeCamera::FreeCamera(
	DirectX::XMFLOAT3 position,
	float moveSpeed,
	float mouseLookSpeed,
	float fieldOfView,
	float aspectRatio,
	float nearClip,
	float farClip,
	CameraProjectionType projType) 
	:
	movementSpeed(moveSpeed),
	mouseLookSpeed(mouseLookSpeed),
	fieldOfView(fieldOfView), 
	aspectRatio(aspectRatio),
	nearClip(nearClip),
	farClip(farClip),
	projectionType(projType)
{
	transform.SetPosition(position);

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

// Nothing to really do
FreeCamera::~FreeCamera()
{ }


// Camera's update, which looks for key presses
void FreeCamera::Update(float dt)
{
	// Current speed
	float speed = dt * movementSpeed;

	// Get the input manager instance
	Input& input = Input::GetInstance();

	// Speed up or down as necessary
	if (input.KeyDown(VK_SHIFT)) { speed *= 5; }
	if (input.KeyDown(VK_CONTROL)) { speed *= 0.1f; }

	// Movement
	if (input.KeyDown('W')) { transform.MoveRelative(0, 0, speed); }
	if (input.KeyDown('S')) { transform.MoveRelative(0, 0, -speed); }
	if (input.KeyDown('A')) { transform.MoveRelative(-speed, 0, 0); }
	if (input.KeyDown('D')) { transform.MoveRelative(speed, 0, 0); }
	if (input.KeyDown('X')) { transform.MoveAbsolute(0, -speed, 0); }
	if (input.KeyDown(' ')) { transform.MoveAbsolute(0, speed, 0); }

	// Handle mouse movement only when button is down
	if (input.MouseLeftDown())
	{
		// Calculate cursor change
		float xDiff = mouseLookSpeed * input.GetMouseXDelta();
		float yDiff = mouseLookSpeed * input.GetMouseYDelta();
		transform.Rotate(yDiff, xDiff, 0);

		// Clamp the X rotation
		XMFLOAT3 rot = transform.GetPitchYawRoll();
		if (rot.x > XM_PIDIV2) rot.x = XM_PIDIV2;
		if (rot.x < -XM_PIDIV2) rot.x = -XM_PIDIV2;
		transform.SetRotation(rot);
	}

	// Update the view every frame - could be optimized
	UpdateViewMatrix();

}

// Creates a new view matrix based on current position and orientation
void FreeCamera::UpdateViewMatrix()
{
	// Get the camera's forward vector and position
	XMFLOAT3 forward = transform.GetForward();
	XMFLOAT3 pos = transform.GetPosition();

	// Make the view matrix and save
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&pos),
		XMLoadFloat3(&forward),
		XMVectorSet(0, 1, 0, 0)); // World up axis
	XMStoreFloat4x4(&viewMatrix, view);
}

// Updates the projection matrix
void FreeCamera::UpdateProjectionMatrix(float aspectRatio)
{
	this->aspectRatio = aspectRatio;

	XMMATRIX P;

	// Which type?
	if (projectionType == CameraProjectionType::Perspective)
	{
		P = XMMatrixPerspectiveFovLH(
			fieldOfView,		// Field of View Angle
			aspectRatio,		// Aspect ratio
			nearClip,			// Near clip plane distance
			farClip);			// Far clip plane distance
	}
	else // CameraProjectionType::ORTHOGRAPHIC
	{
		P = XMMatrixOrthographicLH(
			orthographicWidth,	// Projection width (in world units)
			orthographicWidth / aspectRatio,// Projection height (in world units)
			nearClip,			// Near clip plane distance 
			farClip);			// Far clip plane distance
	}

	XMStoreFloat4x4(&projMatrix, P);
}

DirectX::XMFLOAT4X4 FreeCamera::GetView() { return viewMatrix; }
DirectX::XMFLOAT4X4 FreeCamera::GetProjection() { return projMatrix; }
Transform* FreeCamera::GetTransform() { return &transform; }

float FreeCamera::GetAspectRatio() { return aspectRatio; }

float FreeCamera::GetFieldOfView() { return fieldOfView; }
void FreeCamera::SetFieldOfView(float fov) 
{ 
	fieldOfView = fov; 
	UpdateProjectionMatrix(aspectRatio);
}

float FreeCamera::GetMovementSpeed() { return movementSpeed; }
void FreeCamera::SetMovementSpeed(float speed) { movementSpeed = speed; }

float FreeCamera::GetMouseLookSpeed() { return mouseLookSpeed; }
void FreeCamera::SetMouseLookSpeed(float speed) { mouseLookSpeed = speed; }

float FreeCamera::GetNearClip() { return nearClip; }
void FreeCamera::SetNearClip(float distance) 
{ 
	nearClip = distance;
	UpdateProjectionMatrix(aspectRatio);
}

float FreeCamera::GetFarClip() { return farClip; }
void FreeCamera::SetFarClip(float distance) 
{ 
	farClip = distance;
	UpdateProjectionMatrix(aspectRatio);
}

float FreeCamera::GetOrthographicWidth() { return orthographicWidth; }
void FreeCamera::SetOrthographicWidth(float width)
{
	orthographicWidth = width;
	UpdateProjectionMatrix(aspectRatio);
}

CameraProjectionType FreeCamera::GetProjectionType() { return projectionType; }
void FreeCamera::SetProjectionType(CameraProjectionType type) 
{
	projectionType = type;
	UpdateProjectionMatrix(aspectRatio);
} 


