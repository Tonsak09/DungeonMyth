#pragma once

#include <wrl/client.h>
#include <DirectXMath.h>
#include <memory>
#include "Mesh.h"
#include "Transform.h"
#include "Camera.h"
#include "Material.h"

class GameEntity
{
public:
	GameEntity(std::shared_ptr<Mesh> mesh, std::shared_ptr<RendMat> material);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<RendMat> GetMaterial();
	Transform* GetTransform();

	void SetMesh(std::shared_ptr<Mesh> mesh);
	void SetMaterial(std::shared_ptr<RendMat> material);

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
		std::shared_ptr<SimplePixelShader> inPS,
		Camera* camera);

private:

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<RendMat> material;
	Transform transform;
};

