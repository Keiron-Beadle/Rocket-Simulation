#pragma once
#include <chrono>

class Timer final
{
private:
	Timer();
	std::chrono::high_resolution_clock::time_point _lastTime;
	std::chrono::high_resolution_clock::time_point _lastTimeChange;
	float _elapsedT;
	float _deltaT = 0.166f;
	float _timeMultiplier = 1.0f;
public:
	~Timer() = default;
	Timer(const Timer&) = delete;
	Timer& operator=(const Timer&) = delete;

	static Timer& getInstance() {
		static Timer instance;
		return instance;
	}
	void tick();
	void speedUp();
	void slowDown();
	const inline float delta() const { return _deltaT; }
	const inline float elapsed() const { return _elapsedT; }
};

