#include "GameEntity.h"

using namespace DirectX;

GameEntity::GameEntity(
	std::shared_ptr<Mesh> mesh, 
	std::shared_ptr<RendMat> material) :
	mesh(mesh),
	material(material)
{
}

std::shared_ptr<Mesh> GameEntity::GetMesh() { return mesh; }
std::shared_ptr<RendMat> GameEntity::GetMaterial() { return material; }
Transform* GameEntity::GetTransform() { return &transform; }

void GameEntity::SetMesh(std::shared_ptr<Mesh> mesh) { this->mesh = mesh; }
void GameEntity::SetMaterial(std::shared_ptr<RendMat> material) { this->material = material; }


//void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<FreeCamera> camera)
//{
//	// Set up the material (shaders)
//	material->PrepareMaterial(&transform, camera);
//
//	// Draw the mesh
//	mesh->SetBuffersAndDraw(context);
//}
//
//void GameEntity::Draw(
//	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, 
//	Camera* camera)
//{
//	// Set up the material (shaders)
//	//material->PrepareMaterial(&transform, camera);
//
//	// Sets the texture SRVs and smaples of the material 
//	material->PrepareMaterial();
//
//	// Draw the mesh
//	mesh->SetBuffersAndDraw(context);
//}

void GameEntity::Draw(
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	std::shared_ptr<SimplePixelShader> inPS,
	Camera* camera)
{
	// Set up the material (shaders)
	//material->PrepareMaterial(&transform, camera);

	// Sets the texture SRVs and smaples of the material 
	//material->PrepareMaterial(inPS);
	PrepareMaterial(material, inPS);

	// Draw the mesh
	mesh->SetBuffersAndDraw(context);
}