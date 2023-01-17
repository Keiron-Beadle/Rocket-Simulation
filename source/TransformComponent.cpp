#include "TransformComponent.h"

TransformComponent::TransformComponent() : AComponent(COMPONENT_TRANSFORM), _position(XMFLOAT3(0,0,0)), _scale(XMFLOAT3(0,0,0)),
	_orientation(XMFLOAT3(0,0,0))
{
}

TransformComponent::TransformComponent(XMFLOAT3 p , XMFLOAT3 o, XMFLOAT3 s, std::shared_ptr<TransformComponent> parent)
	: AComponent(COMPONENT_TRANSFORM), _position(p), _oPosition(p), _scale(s), _oScale(s), _orientation(o), _oOrientation(o), _parent(parent)
{
	getTransform();
}

TransformComponent::TransformComponent(const TransformComponent& tc) : AComponent(COMPONENT_TRANSFORM)
{
	deepCopy(tc);
}

TransformComponent::TransformComponent(TransformComponent&& tc) noexcept : AComponent(COMPONENT_TRANSFORM)
{
	moveCopy(std::move(tc));
}

TransformComponent& TransformComponent::operator=(TransformComponent&& tc) noexcept {
	if (this == &tc) return *this;
	moveCopy(std::move(tc));
	return *this;
}

TransformComponent& TransformComponent::operator=(const TransformComponent& tc)
{
	if (this == &tc) return *this;
	deepCopy(tc);
	return *this;
}

const XMFLOAT3X3& TransformComponent::getTransform() {
	XMMATRIX transform = getTransformAligned();
	XMStoreFloat3x3(&_transform, transform);
	return _transform;
}

const XMMATRIX TransformComponent::getTransformAligned() const {
	auto rot = _orientation;
	auto rotX = XMMatrixRotationX(rot.x);
	auto rotY = XMMatrixRotationY(rot.y);
	auto rotZ = XMMatrixRotationZ(rot.z);
	auto rotMat = rotX * rotY * rotZ;
	auto scale = getScale();
	auto scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	auto translateMat = XMMatrixTranslation(_position.x, _position.y, _position.z);
	auto parentMat = XMMatrixIdentity();
	if (_parent) {
		auto parentRot = _parent->getOrientation();
		auto parentPos = _parent->getPosition();
		auto subMatTrans = XMMatrixTranslationFromVector(XMVectorSubtract(XMLoadFloat3(&_position), XMLoadFloat3(&parentPos)));
		auto posMatTrans = XMMatrixTranslationFromVector(XMVectorAdd(XMLoadFloat3(&_position), XMLoadFloat3(&parentPos)));
		auto pRotX = XMMatrixRotationX(parentRot.x);
		auto pRotY = XMMatrixRotationY(parentRot.y);
		auto pRotZ = XMMatrixRotationZ(parentRot.z);
		auto pRotMat = pRotX * pRotY * pRotZ;
		return translateMat * pRotMat * scaleMat * XMMatrixTranslationFromVector(XMLoadFloat3(&parentPos));
	}

	return rotMat * scaleMat * translateMat;
}

void TransformComponent::onPropertyChanged(const AComponent& component)
{
	if (component.getType() == COMPONENT_TRANSFORM) {
		notify(); //Notify children of this node if this node's parent has updated.
	}
}

void TransformComponent::rotate(const XMFLOAT3& axis, const float angle)
{
	rotate(axis, angle, _position);
}

void TransformComponent::resetTransform()
{
	_position = _oPosition;
	_orientation = _oOrientation;
	_scale = _oScale;
	notify();
}

void TransformComponent::rotatePosition(const XMFLOAT3& axis, const float angle) {
	auto rotMat = XMMatrixRotationAxis(XMLoadFloat3(&axis), angle);
	XMStoreFloat3(&_position, XMVector3Transform(XMLoadFloat3(&_position), rotMat));
}

void TransformComponent::rotate(const XMFLOAT3& axis, const float angle, const XMFLOAT3 origin)
{
	XMStoreFloat3(&_orientation,XMVectorAdd(XMLoadFloat3(&_orientation), XMVectorScale(XMLoadFloat3(&axis), angle)));
}

void TransformComponent::move(const XMFLOAT3& desiredMovement)
{
	auto pos = XMLoadFloat3(&_position);
	auto mov = XMLoadFloat3(&desiredMovement);
	auto newPos = XMVectorAdd(pos, mov);
	XMStoreFloat3(&_position, newPos);
	notify();
}

void TransformComponent::onAwake(Entity& e, const CComPtr<ID3D11Device>& device)
{
	if (_parent) {
		_parent->addObserver(weak_from_this());
	}
}

void TransformComponent::deepCopy(const TransformComponent& tc)
{
	_position = tc._position;
	_oPosition = tc._oPosition;
	_orientation = tc._orientation;
	_oOrientation = tc._oOrientation;
	_scale = tc._scale;
	_oScale = tc._oScale;
	_transform = tc._transform;
	_parent = tc._parent;
}

void TransformComponent::moveCopy(TransformComponent&& tc) noexcept
{
	_position = tc._position;
	_oPosition = tc._oPosition;
	_orientation = tc._orientation;
	_oOrientation = tc._oOrientation;
	_scale = tc._scale;
	_oScale = tc._oScale;
	_transform = tc._transform;
	tc._parent.swap(_parent);
}