#include "app.h"
#include "Managers/ResourceManager.h"
#include "Managers/CameraManager.h"

App::App(HWND& hwnd) : _hWnd(hwnd),
	_d3dManager(std::make_shared<DirectX11Manager>(_hWnd)) {

	auto& resourceManager = ResourceManager::getInstance();
	resourceManager.loadResourcesFromJson("RocketSimConfig.json", _d3dManager->getDevice(), _d3dManager->getContext());

	//Setup d3d11 renderer
	auto renderer = std::make_shared<DirectX11Renderer>();
	renderer->setDirectXModules(_d3dManager);
	_renderSystems.push_back(renderer);
	//

	//Setup d3d11 physics
	_physicsSystem = std::make_shared<DirectX11Physics>();
	_updateSystems.push_back(_physicsSystem);
	_collisionSystem = std::make_shared<DirectX11Collision>(_d3dManager->getContext());
	_updateSystems.push_back(_collisionSystem);
	//

	const auto& entities = resourceManager.getEntities();
	//Awake all entities.
	for (const auto& e : entities)
		e->awake(_d3dManager->getDevice());

	//Pass awoken entities to managers/systems
	CameraManager::getInstance().onInit(entities);
	for (const auto& s : _updateSystems)
		s->onInit(entities);
	for (const auto& s : _renderSystems)
		s->onInit(entities);

}

void App::onCamMove(const int key)
{
	CameraManager::getInstance().move(key, Timer::getInstance().delta());
}

void App::changeRenderMode()
{
	//Bit of hard coded right now. 
	auto renderer = dynamic_cast<DirectX11Renderer*>(_renderSystems[0].get());
	renderer->changeRenderMode();
}

void App::changeMRTMode()
{
	auto renderer = dynamic_cast<DirectX11Renderer*>(_renderSystems[0].get());
	renderer->changeMRTMode();
}

void App::slowDown()
{
	Timer::getInstance().slowDown();
}

void App::speedUp()
{
	Timer::getInstance().speedUp();
}

void App::changeLauncherPitch(const int amount)
{
	_physicsSystem->changeRocketPitch(amount);
}

void App::selectCamera(const int cam)
{
	CameraManager::getInstance().setCamera(cam);
}

void App::onCamRotate(const int key)
{
	CameraManager::getInstance().rotate(key, Timer::getInstance().delta());
}

void App::run()
{
	Timer::getInstance().tick();

	for (const auto& s : _updateSystems)
		s->onAction();

	for (const auto& s : _renderSystems)
		s->onAction();

	if (_collisionSystem->hasRocketCollided()) {
		_physicsSystem->resetRocketPosition();
		_collisionSystem->resetRocketCollision();
	}
}

void App::fireRocket()
{
	_physicsSystem->fire();
}

