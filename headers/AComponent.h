#pragma once
#include <vector>
#include <memory>
#include <atlbase.h>
#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;

enum ComponentType {
	COMPONENT_NONE = 0,
	COMPONENT_TRANSFORM = 1 << 0,
	COMPONENT_GEOMETRY = 1 << 1,
	COMPONENT_TEXTURE = 1 << 2,
	COMPONENT_RENDER = 1 << 3,
	COMPONENT_TERRAIN = 1 << 4,
	COMPONENT_SHADER = 1 << 5,
	COMPONENT_COLLIDER = 1 << 6,
	COMPONENT_CAMERA = 1 << 7,
	COMPONENT_LIGHT = 1 << 8,
	COMPONENT_ANIMATION = 1 << 9,
	COMPONENT_EMITTER = 1 << 10
};

class Entity;
class AComponent : public std::enable_shared_from_this<AComponent> {
	friend Entity;
private:
	ComponentType _type;
	std::vector<std::weak_ptr<AComponent>> _observers;
public:
	AComponent(ComponentType type);
	virtual ~AComponent() {};
	AComponent(const AComponent&);
	AComponent& operator=(const AComponent&);
	AComponent(AComponent&&);
	AComponent& operator=(AComponent&&);

	void notify();
	virtual void onAwake(Entity& e, const CComPtr<ID3D11Device>& device) = 0;
	void addObserver(const std::weak_ptr<AComponent> observer);
	void removeObserver(const std::weak_ptr<AComponent> observer);
	const ComponentType getType() const { return _type; }
protected:
	virtual void onPropertyChanged(const AComponent& component) = 0;
};