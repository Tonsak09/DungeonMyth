#include "Renderer.h"

void Renderer::DrawToTargetBuffer(
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> targetBuffer,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
	std::shared_ptr<Camera> cam)
{
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
				cam.get(),
				shadowViewMatrix,
				shadowProjectionMatrix);

			// Draw entity 
			entity->Draw(context, ps);
		}
	}

	// Draw the sky
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	sky->Draw(cam.get());

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