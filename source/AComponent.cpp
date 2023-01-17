#include "AComponent.h"

AComponent::AComponent(ComponentType type) : _type(type)
{
}

void AComponent::addObserver(const std::weak_ptr<AComponent> observer) {
	_observers.push_back(observer);
}

void AComponent::removeObserver(const std::weak_ptr<AComponent> observer) {
	auto locked = observer.lock();
	for (int i = 0; i < _observers.size(); ++i)
		if (_observers[i].lock().get() == locked.get())
			_observers.erase(_observers.begin() + i);
}

void AComponent::notify() {
	for (auto o : _observers)
		o.lock()->onPropertyChanged(*this);
}