#pragma once
#include "AComponent.h"
#include <DirectXMath.h>

using namespace DirectX;
class TransformComponent : public AComponent {
private:
	XMFLOAT3 _position, _oPosition; 
	XMFLOAT3 _orientation, _oOrientation;
	XMFLOAT3 _scale, _oScale;
	XMFLOAT3X3 _transform;
	std::shared_ptr<TransformComponent> _parent;
public:
	TransformComponent();
	TransformComponent(XMFLOAT3, XMFLOAT3, XMFLOAT3, std::shared_ptr<TransformComponent>);
	~TransformComponent() = default;
	TransformComponent(const TransformComponent&);
	TransformComponent& operator=(const TransformComponent&);
	TransformComponent(TransformComponent&&) noexcept;
	TransformComponent& operator=(TransformComponent&&) noexcept;

	const inline XMFLOAT3 getPosition() const { auto p = _parent; return p ? XMFLOAT3(p->_position.x + _position.x, p->_position.y + _position.y, p->_position.z + _position.z) : _position; }
	const inline XMFLOAT3& getLocalPosition() const { return _position; }
	const inline XMFLOAT3 getOrientation() const { auto p = _parent; return p ? XMFLOAT3(p->_orientation.x + _orientation.x, p->_orientation.y + _orientation.y, p->_orientation.z + _orientation.z) : _orientation; }
	const inline XMFLOAT3 getScale() const { auto p = _parent; return p ? XMFLOAT3(p->_scale.x + _scale.x, p->_scale.y + _scale.y, p->_scale.z + _scale.z) : _scale; }
	const XMFLOAT3X3& getTransform();
	const XMMATRIX getTransformAligned() const;
	const inline std::shared_ptr<TransformComponent> getParent() const { return _parent; }

	void resetTransform();
	void rotatePosition(const XMFLOAT3& axis, const float angle);
	void rotate(const XMFLOAT3& axis, const float angle);
	void rotate(const XMFLOAT3& axis, const float angle, const XMFLOAT3 point);
	void move(const XMFLOAT3& desiredMovement);

	void onAwake(Entity& e, const CComPtr<ID3D11Device>& device);
protected:
	void onPropertyChanged(const AComponent& component);
private:
	void deepCopy(const TransformComponent&);
	void moveCopy(TransformComponent&&) noexcept;
};