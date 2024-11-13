#include "Renderer.h"

Renderer::Renderer(
	// TODO: Generate internally 
	Microsoft::WRL::ComPtr<IDXGISwapChain>		swapChain,
	Microsoft::WRL::ComPtr<ID3D11Device>		device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>	context,

	bool vsync,
	bool deviceSupportsTearing,

	// TODO: Generate internally 
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowTextureSRV,
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV,
	DirectX::XMFLOAT4X4 shadowViewMatrix,
	DirectX::XMFLOAT4X4 shadowProjectionMatrix,

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer,
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler,

	std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups,
	std::vector<std::shared_ptr<GameEntity>> entities,
	std::shared_ptr<Sky> sky,

	// TODO: Generate internally 
	std::unordered_map<const wchar_t*, std::shared_ptr<SimpleVertexShader>> nameToVS,
	std::unordered_map<const wchar_t*, std::shared_ptr<SimplePixelShader>> nameToPS,
	std::unordered_map<const wchar_t*, std::shared_ptr<RendMat>> nameToMat):
	swapChain(swapChain),
	device(device),
	context(context),
	vsync(vsync),
	deviceSupportsTearing(deviceSupportsTearing),
	isFullscreen(false),
	shadowDSV(shadowDSV),
	shadowTextureSRV(shadowTextureSRV),
	shadowSRV(shadowSRV),
	shadowViewMatrix(shadowViewMatrix),
	shadowProjectionMatrix(shadowProjectionMatrix),
	shadowRasterizer(shadowRasterizer),
	shadowSampler(shadowSampler),
	entityGroups(entityGroups),
	entities(entities),
	sky(sky),
	nameToVS(nameToVS),
	nameToPS(nameToPS),
	nameToMat(nameToMat)
{

}


// --------------------------------------------------------
// Before rendering the main primary entities go through 
// and draw the shadow depths for sampling later 
// --------------------------------------------------------
	void Renderer::DrawShadowMap(
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetBuffer,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
	{
		// TODO: This does not actually need to be drawn every loop.
		//		 Since our static objects do not actually move around
		//		 and the lighting also does not move then we can continue
		//		 to reuse the same shadow map 

		// Set to shadow rasterizer 
		context->RSSetState(shadowRasterizer.Get());

		// Clear shadow map 
		context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		// Set up shadow depth target. We do not need
		// a color output so null for render target
		ID3D11RenderTargetView* nullRTV{};
		context->OMSetRenderTargets(1, &nullRTV, shadowDSV.Get());

		// Unbind pixel shader 
		context->PSSetShader(0, 0, 0);

		// Adjust viewport to match shadow map resolution 
		D3D11_VIEWPORT viewport = {};
		viewport.Width = (float)SHADOW_MAP_RESOLUTION;
		viewport.Height = (float)SHADOW_MAP_RESOLUTION;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		context->RSSetViewports(1, &viewport);


		// Draw all entities to shadow map 
		std::shared_ptr<SimpleVertexShader> shadowVS = nameToVS[L"ShadowVertex.cso"];
		shadowVS->SetShader();
		shadowVS->SetMatrix4x4("view", shadowViewMatrix);
		shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
		// Loop and draw all entities
		for (auto& e : entities)
		{
			if (!e->castsShadows)
				continue;

			shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
			shadowVS->CopyAllBufferData();


			// Draw the mesh directly to avoid the entity's material
			// Note: Your code may differ significantly here!
			e->GetMesh()->SetBuffersAndDraw(context);
		}

		// Disable shadow rasterizer 
		context->RSSetState(0);

		// Reset pipeline
		viewport.TopLeftX = -(windowWidth - targetSizeX) / 2.0f;
		viewport.TopLeftY = -(windowHeight - targetSizeY) / 2.0f;
		viewport.Width = (float)this->windowWidth;
		viewport.Height = (float)this->windowHeight;
		context->RSSetViewports(1, &viewport);
		context->OMSetRenderTargets(
			1,
			targetBuffer.GetAddressOf(),
			depthBufferDSV.Get());
	}

void Renderer::DrawToTargetBuffer(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetBuffer,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	Camera* cam)
{

	DrawShadowMap(targetBuffer, depthBufferDSV);

	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black
		context->ClearRenderTargetView(targetBuffer.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for (auto group : entityGroups)
	{
		// Pixel shader is set for entire group 
		// since they are (currently) not entity dependent 

		std::shared_ptr<SimplePixelShader> ps = nameToPS[group[0]->GetMaterial()->psName];



		SetPixelShader(
			group[0]->GetMaterial(),
			ps,
			dirLight,
			cam->transform.GetPosition(),
			shadowTextureSRV,
			shadowSRV, shadowSampler,
			psNameToID
		);

		// TODO: Optimization is having setting of the shaders not 
		//		 reset repeated data. We can have a stored enum to 
		//		 remember the previously set shader and pass in a
		//		 bool that indicates whether to use set an entire
		//		 new shader or simply update necessary items 

		for (auto entity : group)
		{
			// Vertex shader must be set for entire group
			// since they are entity dependent 
			SetVertexShader(
				nameToVS[entity->GetMaterial()->vsName],
				entity->GetTransform(),
				cam,
				shadowViewMatrix,
				shadowProjectionMatrix);

			// Draw entity 
			entity->Draw(context, ps);
		}
	}

	// Draw the sky
	sky->Draw(cam);

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, targetBuffer.GetAddressOf(), depthBufferDSV.Get());

		// Unbind shadowmap 
		ID3D11ShaderResourceView* nullSRVs[128] = {};
		context->PSSetShaderResources(0, 128, nullSRVs);
	}
}

void Renderer::Resize(
	unsigned int _windowWidth, unsigned int _windowHeight,
	float _targetSizeX, float _targetSizeY )
{
	windowWidth  = _windowWidth;
	windowHeight = _windowHeight;

	targetSizeX = _targetSizeX;
	targetSizeY = _targetSizeY;
}