#include <DirectXColors.h>
#include "DirectX11Renderer.h"
#include "../Components/ComponentDefinitions.h"
#include "../Managers/CameraManager.h"

#define DEBUG_PARTICLE_SYSTEM

DirectX11Renderer::DirectX11Renderer() 
	: ASystem(static_cast<ComponentType>(COMPONENT_GEOMETRY |
										COMPONENT_RENDER    |
										COMPONENT_SHADER    |
										COMPONENT_TRANSFORM)), _renderMode(RENDER_MODE::WIREFRAME), _mrtMode(MRT_MODE::DEFAULT)
{}


void DirectX11Renderer::onInit(const std::vector<std::shared_ptr<Entity>>& entities) {
	const auto passMask = COMPONENT_GEOMETRY | COMPONENT_SHADER;
	for (const auto& entity : entities) {
		auto mask = getMask();
		auto entityMask = entity->getMask();
		if ((entityMask & mask) >= mask)
			_entities.push_back(entity);

		if ((entityMask & COMPONENT_LIGHT) >= COMPONENT_LIGHT)
			_lights.push_back(entity);

		if ((entityMask & passMask) == passMask)
			_passes.push_back(entity);

		if ((entityMask & COMPONENT_EMITTER) >= COMPONENT_EMITTER)
			_particleSystems.push_back(entity);

		if ((entityMask & (mask | COMPONENT_TERRAIN)) >= (mask | COMPONENT_TERRAIN))
		{
			auto& terrain = entity->getComponent<TerrainComponent>(COMPONENT_TERRAIN);
			terrain.lock()->updateInstanceBuffer(_context);
		}
	}

	//Load shadow shaders
	auto layoutInst = _entities[0].lock()->getComponent<ShaderComponent>(COMPONENT_SHADER).lock()->getVertexLayout();
	auto layoutNoInst = _entities[1].lock()->getComponent<ShaderComponent>(COMPONENT_SHADER).lock()->getVertexLayout();

	_sunlight = std::make_unique<ShadowMap>(_device);
	_sunlight->addShader(_device, "Shaders\\SunShadowInst.hlsli");
	_sunlight->addShader(_device, "Shaders\\SunShadowNoInst.hlsli");
	_sunlight->setLayout(0, layoutInst);
	_sunlight->setLayout(1, layoutNoInst);

	_moonlight = std::make_unique<ShadowMap>(_device);
	_moonlight->addShader(_device, "Shaders\\MoonShadowInst.hlsli");
	_moonlight->addShader(_device, "Shaders\\MoonShadowNoInst.hlsli");
	_moonlight->setLayout(0, layoutInst);
	_moonlight->setLayout(1, layoutNoInst);

	createConstantBuffers();
	createRasterStates();
	_cRenderStateBuffer.misc = XMFLOAT4(1,0,0,0);
	_cUpdateBuffer.dt = XMFLOAT2(0, 0);
	_cUpdateBuffer.t = XMFLOAT2(0, 0);
	_cBlurPassBuffer.misc = XMFLOAT4(0, 0, 0, 0);
	_cMRTBuffer.misc = XMFLOAT4(0, 0, 0, 0);

	_cDrawBuffer.misc.x = 1.0f;
	_cDrawBuffer.misc.w = 0.0f; //This won't be used, but defensive programming
	for (int i = 0; i < 2; ++i) {
		auto lightEntity = _lights[i].lock();
		auto light = lightEntity->getComponent<LightComponent>(COMPONENT_LIGHT).lock();
		auto transform = lightEntity->getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock();
		const auto& lightPos = transform->getPosition();
		const auto& lightAmb = light->getAmbient();
		const float intensity = light->getIntensity();
		const auto& lightDiff = light->getDiffuse();
		const auto& lightSpec = light->getSpecular();
		const auto& lightAtt = light->getAttenuation();
		_cLightBuffer.light[i].position = XMFLOAT4(lightPos.x, lightPos.y, lightPos.z, 1);
		_cLightBuffer.light[i].ambient = XMFLOAT4(lightAmb.x, lightAmb.y, lightAmb.z, intensity);
		_cLightBuffer.light[i].diffuse = XMFLOAT4(lightDiff.x, lightDiff.y, lightDiff.z, 1);
		_cLightBuffer.light[i].specular = XMFLOAT4(lightSpec.x, lightSpec.y, lightSpec.z, 1);
		_cLightBuffer.light[i].attenuation = XMFLOAT4(lightAtt.x, lightAtt.y, lightAtt.z, 1);
	}
	_cLightBuffer.currentLightCount.x = 1;
	_cMRTBuffer.misc.x = static_cast<float>(_mrtMode);
	updateD11Buffer(_gcLightBuffer.p, _cLightBuffer, _context.p);
	updateD11Buffer(_gcMRTBuffer.p, _cMRTBuffer, _context.p);
}

void DirectX11Renderer::onAction() {
	if (!_swapChain) { throw std::exception("Swap chain not set in DirectX11Renderer. Try calling 'setSwapChain'"); }
	
	_context->ClearRenderTargetView(_renderTargetView.p, DirectX::Colors::CornflowerBlue);
	_gbuffer.clear(_context);
	_context->ClearRenderTargetView(_lightPassOutput.getRTV().p, DirectX::Colors::CornflowerBlue);
	_context->ClearRenderTargetView(_blurPassOutput.getRTV().p, DirectX::Colors::CornflowerBlue);
	_context->ClearDepthStencilView(_depthStencilView.p, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	_sunlight->clearDepthBuffer(_context);
	_moonlight->clearDepthBuffer(_context);
	//Update cbuffers
	{
		ID3D11Buffer* buffers[6] = { _gcDrawBuffer.p, _gcUpdateBuffer.p, _gcRenderStateCBuffer.p, _gcVPBuffer.p, _gcMRTBuffer.p, _gcLightBuffer.p };
		_context->VSSetConstantBuffers(0, 6, buffers);
		_context->PSSetConstantBuffers(2, 1, &_gcRenderStateCBuffer.p);
		XMMATRIX invV;
		{ // Update Camera Buffer Data
			auto& cameraManager = CameraManager::getInstance();
			auto& camPos = cameraManager.getPosition();
			auto& view = cameraManager.getView();
			auto& proj = cameraManager.getProjection();
			_cUpdateBuffer.camPos = XMFLOAT4(camPos.x, camPos.y, camPos.z, 1.0);
			auto viewAlignedTransposed = XMMatrixTranspose(XMLoadFloat4x4(&view));
			auto projAlignedTransposed = XMMatrixTranspose(XMLoadFloat4x4(&proj));
			XMStoreFloat4x4(&_cVPBuffer.v, viewAlignedTransposed);
			XMStoreFloat4x4(&_cVPBuffer.p, projAlignedTransposed);
			invV = XMMatrixInverse(nullptr, viewAlignedTransposed);
			XMStoreFloat4x4(&_cVPBuffer.invV, invV);
			auto invP = XMMatrixInverse(nullptr, projAlignedTransposed);
			XMStoreFloat4x4(&_cVPBuffer.invP, invP);
		}
		{ // Update Timer Buffer Data
			_cUpdateBuffer.dt.x = Timer::getInstance().delta();
			_cUpdateBuffer.t.x = Timer::getInstance().elapsed();
		}
		{ // Update Light Buffer Data
			for (int i = 0; i < 2; ++i) {
				auto light = _lights[i].lock();
				auto transform = light->getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock();
				auto& pos = transform->getPosition();
				XMStoreFloat4(&_cLightBuffer.light[i].position, XMLoadFloat3(&pos));
				auto lo = transform->getOrientation();
				auto lightOrientationMat = XMMatrixRotationX(lo.x) * XMMatrixRotationY(lo.y) * XMMatrixRotationZ(lo.z);
				auto lightView = XMMatrixLookAtLH(XMLoadFloat4(&_cLightBuffer.light[i].position), XMVectorSet(0, 0, 0, 1), 
					XMVector3Transform(XMVectorSet(0, 1, 0, 1), lightOrientationMat));
				//auto lightProj = XMMatrixPerspectiveFovLH(1.57f, 1.47f, 0.01f, 30.0f);
				XMFLOAT3 centerLS;
				XMStoreFloat3(&centerLS,XMVector3Transform(XMVectorSet(0, 0, 0, 1), lightView));
				constexpr int r = 35;
				auto lightProj = XMMatrixOrthographicOffCenterLH(centerLS.x - r, centerLS.x + r, centerLS.y - r, centerLS.y + r, centerLS.z - r, centerLS.z + r);
				XMStoreFloat4x4(&_cLightBuffer.light[i].viewProj, XMMatrixMultiply(XMMatrixTranspose(lightProj), XMMatrixTranspose(lightView)));
			}
		}
		
		updateD11Buffer(_gcLightBuffer.p, _cLightBuffer, _context.p);
		updateD11Buffer(_gcUpdateBuffer.p, _cUpdateBuffer, _context.p);
		updateD11Buffer(_gcRenderStateCBuffer.p, _cRenderStateBuffer, _context.p);
		updateD11Buffer(_gcVPBuffer.p, _cVPBuffer, _context.p);
	}
	//Draw passes
	{
		doGeometryPass();
		//doAnyParticleSystems();
		_context->OMSetRenderTargets(6, nullRTVs, nullptr);
		_context->PSSetShaderResources(0, 6, nullSRVs);
		_lightPassOutput.bindToRenderTarget(_context);
		_gbuffer.bindShaderResources(_context); // 0 diffuse 1 normal 2 hdr
		_sunlight->bindDepthResourceToShader(_context, 3); // 3 sun depth
		_moonlight->bindDepthResourceToShader(_context, 4); // 4 moon depth

		{
			if (_mrtMode == MRT_MODE::DIFFUSE || _mrtMode == MRT_MODE::NORM 
				|| _mrtMode == MRT_MODE::SUNLIGHT_DEPTH || _mrtMode == MRT_MODE::LIGHT || _mrtMode == MRT_MODE::MOONLIGHT_DEPTH) 
				_context->OMSetRenderTargets(1, &_renderTargetView.p, nullptr);
			doLightPass();
			if (_mrtMode == MRT_MODE::DIFFUSE || _mrtMode == MRT_MODE::NORM || _mrtMode == MRT_MODE::SUNLIGHT_DEPTH || _mrtMode == MRT_MODE::LIGHT || _mrtMode == MRT_MODE::MOONLIGHT_DEPTH)
				goto cleanup;
		}
		
		_brightPassOutput.bindToRenderTarget(_context); 
		_lightPassOutput.generateMips(_context);
		_lightPassOutput.bindToPixelShaderResource(0, _context); // Lit Scene Slot 0 
		doBrightPass();

		_blurPassOutput.bindToRenderTarget(_context);
		_brightPassOutput.bindToPixelShaderResource(1, _context); // Downsampled Bright Slot 1
		doHorizontalBlurPass();
		doVerticalBlurPass();

		_context->OMSetRenderTargets(1, &_renderTargetView.p, nullptr);	
		_blurPassOutput.bindToPixelShaderResource(1, _context); // Blurred Image Slot 1
		doFinalPass();
	}
	cleanup:
	//Present & Cleanup
	{
		if(FAILED(_swapChain->Present(0, 0))) {
			//HRESULT hr = _device->GetDeviceRemovedReason();
			throw std::exception("[E] Presenting scene.");
		}
		_context->OMSetRenderTargets(6, nullRTVs, nullptr);
		_context->PSSetShaderResources(0, 6, nullSRVs);
	}
}

void DirectX11Renderer::changeRenderMode()
{
	switch (++_renderMode) {
	case RENDER_MODE::WIREFRAME:
		_cRenderStateBuffer.misc.y = 0;
		_cRenderStateBuffer.misc.x = 1;
		break;
	case RENDER_MODE::DIFFUSE_NON_TEXTURED:
		_cRenderStateBuffer.misc.x = 0;
		break;
	case RENDER_MODE::DIFFUSE_TEXTURED:
		_cRenderStateBuffer.misc.x = 1;
		break;
	case RENDER_MODE::DIFFUSE_TEXTURED_DISPLACEMENT:
		_cRenderStateBuffer.misc.y = 1;
		break;
	}
	updateD11Buffer(_gcRenderStateCBuffer.p, _cRenderStateBuffer, _context.p);
}

void DirectX11Renderer::changeMRTMode() {
	_cMRTBuffer.misc.x = static_cast<float>(++_mrtMode);
	updateD11Buffer(_gcMRTBuffer.p, _cMRTBuffer, _context.p);
}

void DirectX11Renderer::doAnyParticleSystems() {
	_gbuffer.bindRenderTargets(_context, _depthStencilView);
	_context->OMSetDepthStencilState(_depthDisabledState.p, 0);
	for (const auto& e : _particleSystems) {
		const auto& emitter = e.lock()->getComponent<EmitterComponent>(COMPONENT_EMITTER).lock();

		const auto& shader = e.lock()->getComponent<ShaderComponent>(COMPONENT_SHADER).lock();
		shader->use(_context);

		auto& cameraManager = CameraManager::getInstance();
		auto& view = cameraManager.getView();
		auto& proj = cameraManager.getProjection();
		auto viewT = XMMatrixTranspose(XMLoadFloat4x4(&view));
		auto projT = XMMatrixTranspose(XMLoadFloat4x4(&proj));
		const auto& transform = e.lock()->getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock();
		const auto modelT = XMMatrixTranspose(transform->getTransformAligned());
		auto mvp = projT * viewT * modelT;
		XMStoreFloat4x4(&_cDrawBuffer.m, modelT);
		XMStoreFloat4x4(&_cDrawBuffer.mvp, mvp);
		updateD11Buffer(_gcDrawBuffer.p, _cDrawBuffer, _context.p);

		const auto& emitterStartCol = emitter->getStartColour();
		_cParticleBuffer.startColour = XMFLOAT4(emitterStartCol.x, emitterStartCol.y, emitterStartCol.z, 1);
		const auto& emitterEndCol = emitter->getEndColour();
		_cParticleBuffer.endColour = XMFLOAT4(emitterEndCol.x, emitterEndCol.y, emitterEndCol.z, 1);
		_cParticleBuffer.misc.x = emitter->getVelocity();
		_cParticleBuffer.misc.y = emitter->getLifespan();
		const auto& emitterDirection = emitter->getDirection();
		_cParticleBuffer.direction = XMFLOAT4(emitterDirection.x, emitterDirection.y, emitterDirection.z, 1);
		const auto& emitterPosition = e.lock()->getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock()->getPosition();
		_cParticleBuffer.emitterPosition = XMFLOAT4(emitterPosition.x, emitterPosition.y, emitterPosition.z, 1);
		updateD11Buffer(_gcParticleBuffer, _cParticleBuffer, _context);
		_context->VSSetConstantBuffers(7, 1, &_gcParticleBuffer.p);
		_context->OMSetBlendState(emitter->getBlendState(), nullptr, 0xffffffff);
		const UINT stride = sizeof(SimpleVertex);
		const UINT offset = 0;
		_context->IASetVertexBuffers(0, 1, &emitter->getVertices().p, &stride, &offset);
		const auto particleCount = emitter->getParticleCount();
		_context->Draw(6 * particleCount, 0);
	}
	_context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	_context->OMSetDepthStencilState(nullptr, 0);
}

void DirectX11Renderer::doFinalPass() {
	auto& entity = _passes[2];
	_blurPassOutput.bindToPixelShaderResource(1, _context);
	drawPassQuad(entity);
}

void DirectX11Renderer::doGeometryPass() {
	if (_renderMode != RENDER_MODE::WIREFRAME) { _context->RSSetState(_rasterState); }
	else { _context->RSSetState(_rasterState_WIRE); }
	auto& cameraManager = CameraManager::getInstance();
	auto& view = cameraManager.getView();
	auto& proj = cameraManager.getProjection();
	auto viewT = XMMatrixTranspose(XMLoadFloat4x4(&view));
	auto projT = XMMatrixTranspose(XMLoadFloat4x4(&proj));

	std::shared_ptr<ShaderComponent> _prevShader;
	for (const auto& e : _entities) {
		auto entity = e.lock();
		//Render target is changed for shadows - needs to be reset again before next entity
		_gbuffer.bindRenderTargets(_context, _depthStencilView);
		//Read Render and Set buffers accordingly
		{
 			const auto render = entity->getComponent<RenderComponent>(COMPONENT_RENDER).lock();
			_cDrawBuffer.misc.y = render->isHDR() ? 1.0f : 0.0f;
			_cDrawBuffer.misc.z = render->isAnimated() ? 1.0f : 0.0f;
		}
		//Read Transform and Set M/MVP Buffers
		{
			const auto transform = entity->getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock();
			auto modelT = XMMatrixTranspose(transform->getTransformAligned());
			auto mvp = projT * viewT * modelT;
			XMStoreFloat4x4(&_cDrawBuffer.m, modelT);
			XMStoreFloat4x4(&_cDrawBuffer.mvp, mvp);
			updateD11Buffer(_gcDrawBuffer.p, _cDrawBuffer, _context.p);
			updateD11Buffer(_gcVPBuffer.p, _cVPBuffer, _context.p);
		}
	
		//Set shaders
		{
			const auto shader = entity->getComponent<ShaderComponent>(COMPONENT_SHADER).lock();
			shader->use(_context);
		}

		//Set PS Resources
		{
			const auto texture = entity->getComponent<TextureComponent>(COMPONENT_TEXTURE).lock();
			ID3D11ShaderResourceView* textures[3] = { texture->getAlbedo().p, texture->getNormal().p, texture->getDisplacement().p };
			ID3D11ShaderResourceView* vTextures[1] = { texture->getDisplacement().p };
			_context->PSSetShaderResources(0, 3, textures);
			_context->VSSetShaderResources(0, 1, vTextures);
		}

		//Set vertex/index buffers
		size_t indexCount;
		{
			const auto geometry = entity->getComponent<GeometryComponent>(COMPONENT_GEOMETRY).lock();
			indexCount = geometry->getIndices().size();
			auto& gVertices = geometry->getGVertices();
			auto& gIndices = geometry->getGIndices();
			UINT stride = geometry->getStride();
			UINT offset = 0;
			_context->IASetVertexBuffers(0, 1, &gVertices.p, &stride, &offset);
			_context->IASetIndexBuffer(gIndices.p, DXGI_FORMAT_R32_UINT, 0);
		}

		//If terrain exists, draw instanced - else don't
		{
			const auto terrain = entity->getComponent<TerrainComponent>(COMPONENT_TERRAIN).lock();
			if (terrain)
			{
				terrain->setInstanceBuffer(_context, 1, 6);
				auto instCount = terrain->getInstanceCount();
				_context->DrawIndexedInstanced(indexCount, instCount, 0, 0, 0);
				_sunlight->use(0, _context);
				_sunlight->bindDSVSetNullRenderTarget(_context);
				_context->DrawIndexedInstanced(indexCount, instCount, 0, 0, 0);
				_moonlight->use(0, _context);
				_moonlight->bindDSVSetNullRenderTarget(_context);
				_context->DrawIndexedInstanced(indexCount, instCount, 0, 0, 0);
			}
			else {
				_context->DrawIndexed(indexCount, 0, 0);
				_sunlight->use(1, _context);
				_sunlight->bindDSVSetNullRenderTarget(_context);
				_context->DrawIndexed(indexCount, 0, 0);
				_moonlight->use(1, _context);
				_moonlight->bindDSVSetNullRenderTarget(_context);
				_context->DrawIndexed(indexCount, 0, 0);
			}
		}
	}
	_context->RSSetState(_rasterState_QUAD);

}

void DirectX11Renderer::doLightPass()
{
	auto& entity = _passes[0];
	_context->PSSetConstantBuffers(5, 1, &_gcLightBuffer.p);
	_context->PSSetConstantBuffers(2, 1, &_gcVPBuffer.p);
	_context->PSSetConstantBuffers(0, 1, &_gcDrawBuffer.p);
	drawPassQuad(entity);
}

void DirectX11Renderer::doHorizontalBlurPass()
{
	auto& entity = _passes[1];
	_context->VSSetConstantBuffers(6, 1, &_gcBlurPassBuffer.p);
	_cBlurPassBuffer.misc.x = 1;
	_cBlurPassBuffer.misc.y = 0;
	updateD11Buffer(_gcBlurPassBuffer.p, _cBlurPassBuffer, _context.p);
	drawPassQuad(entity);
}

void DirectX11Renderer::doBrightPass() {
	auto& entity = _passes[3];
	drawPassQuad(entity);
}

void DirectX11Renderer::doVerticalBlurPass()
{
	auto& entity = _passes[1];
	_cBlurPassBuffer.misc.x = 0;
	_cBlurPassBuffer.misc.y = 1;
	updateD11Buffer(_gcBlurPassBuffer.p, _cBlurPassBuffer, _context.p);
	drawPassQuad(entity);
}

void DirectX11Renderer::drawPassQuad(const std::shared_ptr<Entity> e) {
	//Set vertex/index buffers
	size_t indexCount;
	{
		const auto geometry = e->getComponent<GeometryComponent>(COMPONENT_GEOMETRY).lock();
		indexCount = geometry->getIndices().size();
		auto& gVertices = geometry->getGVertices();
		auto& gIndices = geometry->getGIndices();
		UINT stride = geometry->getStride();
		UINT offset = 0;
		_context->IASetVertexBuffers(0, 1, &gVertices.p, &stride, &offset);
		_context->IASetIndexBuffer(gIndices.p, DXGI_FORMAT_R32_UINT, 0);
	}
	//Set shaders
	{
		const auto shader = e->getComponent<ShaderComponent>(COMPONENT_SHADER).lock();
		shader->use(_context);
	}
	_context->DrawIndexed(indexCount, 0, 0);
}

void DirectX11Renderer::createConstantBuffers()
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(UpdateFrameBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = _device->CreateBuffer(&bd, nullptr, &_gcUpdateBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating Update frame Buffer in DirectX11Renderer.cpp");

	bd.ByteWidth = sizeof(DrawFrameBuffer);
	hr = _device->CreateBuffer(&bd, nullptr, &_gcDrawBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating Draw frame Buffer in DirectX11Renderer.cpp");

	bd.ByteWidth = sizeof(MiscCBuffer);
	hr = _device->CreateBuffer(&bd, nullptr, &_gcRenderStateCBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating Render state Buffer in DirectX11Renderer.cpp");

	hr = _device->CreateBuffer(&bd, nullptr, &_gcMRTBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating MRT Buffer in DirectX11Renderer.cpp");

	hr = _device->CreateBuffer(&bd, nullptr, &_gcBlurPassBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating Blur Pass Buffer in DirectX11Renderer.cpp");

	bd.ByteWidth = sizeof(ViewProjBuffer);
	hr = _device->CreateBuffer(&bd, nullptr, &_gcVPBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating VP Buffer in DirectX11Renderer.cpp");

	bd.ByteWidth = sizeof(ParticleBuffer);
	hr = _device->CreateBuffer(&bd, nullptr, &_gcParticleBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating Particle Buffer in DirectX11Renderer.cpp");

	bd.ByteWidth = sizeof(LightCBuffer);
	hr = _device->CreateBuffer(&bd, nullptr, &_gcLightBuffer.p);
	if (FAILED(hr)) throw std::exception("[E] Creating VP Buffer in DirectX11Renderer.cpp");

}	

void DirectX11Renderer::createRasterStates()
{
	D3D11_RASTERIZER_DESC rd{};
	ZeroMemory(&rd, sizeof(rd));
	rd.CullMode = D3D11_CULL_BACK; //Change to BACK
	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.ScissorEnable = false;
	rd.DepthBias = 0;
	rd.DepthBiasClamp = 0.0f;
	rd.DepthClipEnable = true;
	rd.MultisampleEnable = false;
	rd.SlopeScaledDepthBias = 0.0f;
	HRESULT hr = _device->CreateRasterizerState(&rd, &_rasterState_WIRE);
	if (FAILED(hr)) throw std::exception("[E] Creating RasterState.");

	rd.FillMode = D3D11_FILL_SOLID;
	hr = _device->CreateRasterizerState(&rd, &_rasterState);
	if (FAILED(hr)) throw std::exception("[E] Creating RasterState Quad.");

	hr = _device->CreateRasterizerState(&rd, &_rasterState_QUAD);
	if (FAILED(hr)) throw std::exception("[E] Creating RasterState Quad.");
}

void DirectX11Renderer::setDirectXModules(const std::weak_ptr<DirectX11Manager> weakD3dManager)
{ 
	auto manager = weakD3dManager.lock();
	_swapChain = manager->getSwapChain();
	_device = manager->getDevice();
	_context = manager->getContext();
	UINT width = manager->getWidth();
	UINT height = manager->getHeight();
	
	D3D11_VIEWPORT vp = {};
	vp.Width = static_cast<FLOAT>(width);
	vp.Height = static_cast<FLOAT>(height);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	manager->getContext()->RSSetViewports(1, &vp);

	manager->getContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	manager->getContext()->RSSetState(_rasterState);

	D3D11_TEXTURE2D_DESC depthTextureDesc = {};
	ZeroMemory(&depthTextureDesc, sizeof(depthTextureDesc));
	depthTextureDesc.Width = width;
	depthTextureDesc.Height = height;
	depthTextureDesc.MipLevels = 1;
	depthTextureDesc.ArraySize = 1;
	depthTextureDesc.SampleDesc.Count = 1;
	depthTextureDesc.SampleDesc.Quality = 0;
	depthTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	depthTextureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* DepthStencilTexture = {};
	HRESULT hr = _device->CreateTexture2D(&depthTextureDesc, NULL, &DepthStencilTexture);
	if (FAILED(hr)) throw std::exception("[E] Creating depth stencil texture DirectX11Renderer.");

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = depthTextureDesc.Format;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	hr = _device->CreateDepthStencilView(DepthStencilTexture, &dsvDesc, &_depthStencilView.p);
	DepthStencilTexture->Release();
	if (FAILED(hr)) throw std::exception("[E] Creating Depth stencil view DirectX11Renderer.");

	CComPtr<ID3D11Texture2D> backBuffer;
	hr = _swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer.p));
	if (FAILED(hr)) throw std::exception("[E] Getting backbuffer from swapchain DirectX11Renderer");

	hr = _device->CreateRenderTargetView(backBuffer, nullptr, &_renderTargetView.p);
	if (FAILED(hr)) throw std::exception("[E] Creating RTV from BackBuffer DirectX11Renderer");
	manager->getContext()->OMSetRenderTargets(1, &_renderTargetView.p, nullptr);

	//Create GBuffer
	_gbuffer = GBuffer(3, _device, width, height);

	_cBlurPassBuffer.misc.z = width;
	_cBlurPassBuffer.misc.w = height;
	createGBuffer(_device, width, height, _lightPassOutput);
	createGBuffer(_device, width, height, _blurPassOutput);
	createGBuffer(_device, width, height, _brightPassOutput);


	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	dsDesc.DepthEnable = false;
	hr = _device->CreateDepthStencilState(&dsDesc, &_depthDisabledState.p);
	if (FAILED(hr)) { throw std::exception("[E] Creating depth stencil state in EmitterComponent"); }

}