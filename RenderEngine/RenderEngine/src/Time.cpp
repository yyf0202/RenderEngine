#include "Time.h"

float Engine::Time::deltaTime = 1.0/60.0f;
float Engine::Time::timeSinceBegining = 0.0f;

void Engine::Time::update(double time)
{
	float fltTime = float(time);
	deltaTime = fltTime - timeSinceBegining;
	timeSinceBegining = fltTime;
}