#pragma once
#include <string>
#include <unordered_map>
#include <d3d11.h>
#include <atlbase.h>
#include "Components/AComponent.h"
#include "Components/NoneComponent.h"

class Entity
{
	std::string _name;
	uint16_t _mask;
	uint8_t _id;
	std::unordered_map<ComponentType, std::shared_ptr<AComponent>> _components;
public:
	Entity(const char* name, const uint8_t id);
	~Entity() = default;
	Entity(const Entity&);
	Entity operator=(const Entity&);
	Entity(Entity&&);
	Entity operator=(Entity&&);

	void addComponent(const std::shared_ptr<AComponent> component);
	void removeComponent(ComponentType type);
	void awake(const CComPtr<ID3D11Device>&);
	const std::string& getName() const { return _name; }
	const uint16_t getMask() const { return _mask; }
	const uint8_t getId()  const { return _id; }

	template <class T>
	const std::weak_ptr<T> getComponent(ComponentType type) {
		auto valueItr = _components.find(type);
		if (valueItr == _components.end()) //This is fine I expect "empty" to be returned.
			return std::dynamic_pointer_cast<T>(std::make_shared<NoneComponent>());
		return std::dynamic_pointer_cast<T>(_components.at(type));
	}
};

