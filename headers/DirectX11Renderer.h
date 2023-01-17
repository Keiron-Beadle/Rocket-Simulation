#pragma once
#include "ASystem.h"
#include "../RenderTarget.h"
#include "../ShadowMap.h"
#include "../GBuffer.h"
#include "../Timer.h"
#include "../Utility.h"
#include "../Managers/DirectX11Manager.h"
#include "../Components/ShaderComponent.h"

class DirectX11Renderer : public ASystem {
private:
	std::vector<std::weak_ptr<Entity>> _lights;
	std::vector<std::shared_ptr<Entity>> _passes;
	std::vector<std::weak_ptr<Entity>> _particleSystems;
	CComPtr<IDXGISwapChain> _swapChain = nullptr;
	CComPtr<ID3D11Device> _device = nullptr;
	CComPtr<ID3D11DeviceContext> _context = nullptr;
	CComPtr<ID3D11RenderTargetView> _renderTargetView = nullptr;
	CComPtr<ID3D11DepthStencilView> _depthStencilView = nullptr;
	CComPtr<ID3D11ShaderResourceView> _depthStencilSRV = nullptr;
	CComPtr<ID3D11DepthStencilState> _depthDisabledState = nullptr;
	CComPtr<ID3D11RasterizerState> _rasterState, _rasterState_QUAD, _rasterState_WIRE;
	CComPtr<ID3D11Buffer> _gcUpdateBuffer, _gcDrawBuffer, _gcRenderStateCBuffer, 
		_gcVPBuffer, _gcLightBuffer, _gcBlurPassBuffer, _gcParticleBuffer, _gcMRTBuffer;
	RenderTarget _lightPassOutput, _blurPassOutput, _brightPassOutput;

	std::unique_ptr<ShadowMap> _sunlight, _moonlight;
	GBuffer _gbuffer;
	UINT width, height;
	RENDER_MODE _renderMode;
	MRT_MODE _mrtMode;
	LightCBuffer _cLightBuffer;
	DrawFrameBuffer _cDrawBuffer;
	UpdateFrameBuffer _cUpdateBuffer;
	MiscCBuffer _cRenderStateBuffer, _cBlurPassBuffer, _cMRTBuffer;
	ViewProjBuffer _cVPBuffer;
	ParticleBuffer _cParticleBuffer;
	ID3D11RenderTargetView* nullRTVs[6] = { nullptr,nullptr,nullptr,nullptr, nullptr, nullptr};
	ID3D11ShaderResourceView* nullSRVs[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
public:
	DirectX11Renderer();
	~DirectX11Renderer() = default;
	DirectX11Renderer(const DirectX11Renderer&);
	DirectX11Renderer& operator=(const DirectX11Renderer&);

	void setDirectXModules(const std::weak_ptr<DirectX11Manager>);
	void onInit(const std::vector<std::shared_ptr<Entity>>&) override;
	void onAction() override;
	void changeRenderMode();
	void changeMRTMode();

private:
	void createConstantBuffers();
	void createRasterStates(); 
	void doGeometryPass();
	void doLightPass();
	void doHorizontalBlurPass();
	void doVerticalBlurPass();
	void doBrightPass();
	void doFinalPass();
	void doAnyParticleSystems();
	void drawPassQuad(const std::shared_ptr<Entity>);

};