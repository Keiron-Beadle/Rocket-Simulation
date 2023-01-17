#include "CameraComponent.h"
#include "TransformComponent.h"
#include "../Entity.h"
#include "../constants.h"

CameraComponent::CameraComponent()
	: CameraComponent(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 1, 0))
{
}

CameraComponent::CameraComponent(const CameraComponent& cc) : AComponent(COMPONENT_CAMERA)
{
	deepCopy(cc);
}

CameraComponent& CameraComponent::operator=(const CameraComponent& cc)
{
	if (this == &cc) return *this;
	deepCopy(cc);
	return *this;
}

CameraComponent::CameraComponent(CameraComponent&& cc) noexcept : AComponent(COMPONENT_CAMERA)
{
	moveCopy(std::move(cc));
}

CameraComponent& CameraComponent::operator=(CameraComponent&& cc) noexcept
{
	if (this == &cc) return *this;
	moveCopy(std::move(cc));
	return *this;
}

CameraComponent::CameraComponent(XMFLOAT3 lookAt, XMFLOAT3 up, float fov, float nearPlane, float farPlane)
	: AComponent(COMPONENT_CAMERA), _lookAt(lookAt), _up(up), _fov(fov), _near(nearPlane), _far(farPlane),
	_pos(XMFLOAT3(0,0,0))
{
	updateProjection(Constants::DESIRED_WIDTH, Constants::DESIRED_HEIGHT);
}

void CameraComponent::updateProjection(UINT width, UINT height) {
	 auto projectionM = XMMatrixPerspectiveFovLH(_fov,
	 	static_cast<float>(width) / static_cast<float>(height),
	 	_near, _far
	 );
	//auto projectionM = XMMatrixOrthographicOffCenterLH(-0.8, 0.3, -0.95, 0.05, 0.01f, 10.0f);
	XMStoreFloat4x4(&_projection, projectionM);
}

void CameraComponent::onPropertyChanged(const AComponent& component) {
	if (component.getType() == COMPONENT_TRANSFORM) {
		if (&component == _parent.get()) {
			auto parentPos = _parent->getPosition();
			XMStoreFloat3(&_lookAt,XMLoadFloat3(&parentPos));
			XMStoreFloat3(&_pos, XMVectorAdd(XMLoadFloat3(&parentPos), XMLoadFloat3(&_offset)));
			updateView();
			return;
		}
		auto& transform = dynamic_cast<const TransformComponent&>(component);
		_pos = transform.getPosition();
		updateView();
	}
}

void CameraComponent::onAwake(Entity& e, const CComPtr<ID3D11Device>& device) {
	auto transform = e.getComponent<TransformComponent>(COMPONENT_TRANSFORM).lock();
	_pos = transform->getPosition();
	_offset = transform->getLocalPosition();
	_parent = transform->getParent();
	if (_parent) {
		_parent->addObserver(weak_from_this());
		auto parentPos = _parent->getPosition();
		XMStoreFloat3(&_lookAt, XMLoadFloat3(&parentPos));
	}
	updateView();
	transform->addObserver(weak_from_this());
}

void CameraComponent::updateView() {
	XMVECTOR posV = XMLoadFloat3(&_pos);
	XMVECTOR lookAtV = XMLoadFloat3(&_lookAt);
	XMVECTOR upV = XMLoadFloat3(&_up);
	XMMATRIX viewM = XMMatrixLookAtLH(posV, lookAtV, upV);
	XMStoreFloat4x4(&_view, viewM);
}

void CameraComponent::deepCopy(const CameraComponent& cc)
{
	_view = cc._view;
	_projection = cc._projection;
	_pos = cc._pos;
	_lookAt = cc._lookAt;
	_up = cc._up;
	_offset = cc._offset;
	_fov = cc._fov;
	_near = cc._near;
	_far = cc._far;
	_parent = cc._parent;
}

void CameraComponent::moveCopy(CameraComponent&& cc) noexcept {
	_view = cc._view;
	_projection = cc._projection;
	_pos = cc._pos;
	_lookAt = cc._lookAt;
	_up = cc._up;
	_offset = cc._offset;
	_fov = cc._fov;
	_near = cc._near;
	_far = cc._far;
	cc._parent.swap(_parent);
}
