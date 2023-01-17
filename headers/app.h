#pragma once
#include <memory>
#include "Managers/DirectX11Manager.h"
#include "Systems/ASystem.h"
#include "Systems/DirectX11Renderer.h"
#include "Systems/DirectX11Physics.h"
#include "Systems/DirectX11Collision.h"

class App {
private:
	HWND _hWnd;
	std::shared_ptr<DirectX11Manager> _d3dManager;
	std::vector<std::shared_ptr<ASystem>> _updateSystems;
	std::vector<std::shared_ptr<ASystem>> _renderSystems;
	std::shared_ptr<DirectX11Physics> _physicsSystem;
	std::shared_ptr<DirectX11Collision> _collisionSystem;
public:
	App(HWND& hwnd);
	~App()						= default;
	App(const App&)				= delete;
	App operator=(const App&)	= delete;

	void onCamMove(const int key);
	void changeRenderMode();
	void changeMRTMode();
	void slowDown();
	void speedUp();
	void changeLauncherPitch(const int amount);
	void selectCamera(const int cam);
	void onCamRotate(const int key);
	void run();
	void fireRocket();
};