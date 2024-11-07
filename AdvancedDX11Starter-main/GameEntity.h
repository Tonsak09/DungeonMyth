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
	GameEntity(
		std::shared_ptr<Mesh> mesh, 
		std::shared_ptr<RendMat> material,
		bool castShadows = true);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<RendMat> GetMaterial();
	Transform* GetTransform();

	bool castsShadows;


	void SetMesh(std::shared_ptr<Mesh> mesh);
	void SetMaterial(std::shared_ptr<RendMat> material);

	void Draw(
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimplePixelShader> inPS);

private:

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<RendMat> material;
	Transform transform;
};

