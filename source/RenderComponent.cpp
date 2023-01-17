#include "RenderComponent.h"

RenderComponent::RenderComponent() : AComponent(COMPONENT_RENDER), _animationEnabled(false), _hdrEnabled(false), _renderType(RenderType::Render_Normal)
{
	Material m{};
	m.MaterialAmbient = XMFLOAT4(0, 0, 0,0);
	m.MaterialDiffuse = XMFLOAT4(0, 0, 0,0);
	m.MaterialSpecular = XMFLOAT4(0, 0, 0,0);
	_material = m;
}

RenderComponent::RenderComponent(const RenderComponent& rc) : AComponent(COMPONENT_RENDER)
{
	deepCopy(rc);
}

RenderComponent& RenderComponent::operator=(const RenderComponent& rc)
{
	if (this == &rc) return *this;
	deepCopy(rc);
	return *this;
}

RenderComponent::RenderComponent(Material material, RenderType renderType, bool hdr, bool animation) :
	AComponent(COMPONENT_RENDER), _material(material), _renderType(renderType), _hdrEnabled(hdr), _animationEnabled(animation) 
{

}

void RenderComponent::onAwake(Entity& e, const CComPtr<ID3D11Device>& device)
{
}

void RenderComponent::onPropertyChanged(const AComponent& component)
{
}

void RenderComponent::deepCopy(const RenderComponent& rc)
{
	_material = rc._material;
	_renderType = rc._renderType;
	_hdrEnabled = rc._hdrEnabled;
	_animationEnabled = rc._animationEnabled;
}