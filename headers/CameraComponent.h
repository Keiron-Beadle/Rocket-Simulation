#pragma once
#include "ComponentDefinitions.h"

class CameraComponent : public AComponent {
private:
	XMFLOAT4X4 _view, _projection;
	XMFLOAT3 _pos, _lookAt, _up, _offset;
	float _fov, _near, _far;
	std::shared_ptr<TransformComponent> _parent;
public:
	CameraComponent();
	CameraComponent(XMFLOAT3 lookAt = XMFLOAT3(0,0,0), XMFLOAT3 up = XMFLOAT3(0, 1, 0), float fov = XM_PIDIV2, float nearPlane = 0.01f, float farPlane = 100.0f);
	~CameraComponent() = default;
	CameraComponent(const CameraComponent&);
	CameraComponent& operator=(const CameraComponent&);
	CameraComponent(CameraComponent&&) noexcept;
	CameraComponent& operator=(CameraComponent&&) noexcept;

	void updateView();
	void updateProjection(UINT width, UINT height);
	const inline XMFLOAT4X4& getView() const { return _view; }
	const inline XMFLOAT4X4& getProjection() const { return _projection; }
	const inline XMFLOAT3& getPosition() const { return _pos; }
	const inline XMFLOAT3& getLookAt() const { return _lookAt; }
	const inline XMFLOAT3& getUp() const { return _up; }

	void onAwake(Entity& e, const CComPtr<ID3D11Device>& device);

	void setLookAt(XMFLOAT3 lookAt) { _lookAt = lookAt; updateView(); }
	void setPosition(XMFLOAT3 position) { _pos = position; updateView(); }
protected:
	void onPropertyChanged(const AComponent& component);
private:
	void deepCopy(const CameraComponent&);
	void moveCopy(CameraComponent&&) noexcept;
};