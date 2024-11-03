#pragma once
#include <Windows.h>
#define MAX_FPS_TICK 0.1f

long long GetCurrentTimeMillis();

class FPS
{
public:

	void Init();
	void Update();
	float GetFpsMillis();

private:

	long long pre;
	float fps;
};