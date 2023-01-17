#include "Timer.h"
#include <d3d11.h>

Timer::Timer() {
	_lastTime = std::chrono::high_resolution_clock::now();
}

void Timer::tick()
{
	const auto now = std::chrono::high_resolution_clock::now();
	_deltaT = std::chrono::duration_cast<std::chrono::microseconds> (now - _lastTime).count();
	_deltaT *= 0.000001; //Convert micro to seconds
	_deltaT *= _timeMultiplier; //Speed up / slow down time
	_lastTime = now;
	static ULONGLONG timeStart = 0;
	ULONGLONG timecur = GetTickCount64();
	if (timeStart == 0) { timeStart = timecur; }
	_elapsedT = (timecur - timeStart) / 1000.0f;
}

void Timer::speedUp()
{
	const auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds> (now - _lastTimeChange).count() < 16)
		return;

	_lastTimeChange = now;
	_timeMultiplier += 0.1f;
	
}

void Timer::slowDown()
{
	const auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds> (now - _lastTimeChange).count() < 16)
		return;

	_lastTimeChange = now;
	_timeMultiplier -= 0.1f;
}