#pragma once
#include "AComponent.h"
#include <DirectXMath.h>

using namespace DirectX;

struct Material {
	XMFLOAT4 MaterialAmbient; //Shininess is W component here
	XMFLOAT4 MaterialDiffuse;
	XMFLOAT4 MaterialSpecular;
};

enum RenderType {
	Render_Normal,
	Render_Indexed,
	Render_InstancedIndexed
};

class RenderComponent : public AComponent {
private:
	Material _material;
	RenderType _renderType;
	bool _hdrEnabled;
	bool _animationEnabled;
public:
	RenderComponent();
	RenderComponent(Material material, RenderType renderType, bool hdr, bool animation);
	~RenderComponent() = default;
	RenderComponent(const RenderComponent&);
	RenderComponent& operator=(const RenderComponent&);

	const Material& getMaterial() const { return _material; }
	const bool isHDR() { return _hdrEnabled; }
	const bool isAnimated() { return _animationEnabled; }
	void onAwake(Entity& e, const CComPtr<ID3D11Device>& device);
protected:
	void onPropertyChanged(const AComponent& component);
private:
	void deepCopy(const RenderComponent&);
};