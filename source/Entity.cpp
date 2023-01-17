#include "Entity.h"

Entity::Entity(const char* name, const uint8_t id) : _name(name), _id(id), _mask(0)
{ 
}

void Entity::awake(const CComPtr<ID3D11Device>& device) {
	for (const auto& [key, val] : _components) {
		val->onAwake(*this, device);
	}
}

void Entity::addComponent(const std::shared_ptr<AComponent> component)
{
	auto newComponentType = component->getType();
	if (_components.count(newComponentType) != 0) {
		return;
	}
	_components[newComponentType]=component;
	_mask |= newComponentType;
}

void Entity::removeComponent(ComponentType type)
{
	if (_components.erase(type) != 0) _mask ^= type; 
}