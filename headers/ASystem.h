#pragma once
#include <vector>
#include <string>
#include "../Entity.h"
#include "../Components/AComponent.h"

class ASystem {
private:
	ComponentType _mask;
protected:
	std::vector<std::weak_ptr<Entity>> _entities;
	const inline ComponentType& getMask() const { return _mask; }
	inline std::vector<std::weak_ptr<Entity>>& getMutEntities() { return _entities; }
	const inline std::vector<std::weak_ptr<Entity>>& getEntities() const { return _entities; }
	ASystem(const ComponentType mask);
public:
	virtual ~ASystem() = default;
	ASystem(const ASystem&) = delete;
	ASystem operator=(const ASystem&) = delete;

	virtual void onInit(const std::vector<std::shared_ptr<Entity>>&) = 0;
	virtual void onAction() = 0;
};