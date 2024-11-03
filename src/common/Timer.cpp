#include "common/Timer.h"

long long GetCurrentTimeMillis()
{
#ifdef _WIN64
	return GetTickCount64();
#else
	timeval t;
	gettimeofday(&t, NULL);
	long long ret = t.tv_sec * 1000 + t.tv_usec / 1000;
	return ret;
#endif
}

void FPS::Init()
{
	fps = 0.0f;
	pre = GetCurrentTimeMillis();
}

void FPS::Update()
{
	auto current = GetCurrentTimeMillis();
	float time = (float)(current - pre) / 1000;
	if (time > MAX_FPS_TICK)
		time = MAX_FPS_TICK;
	fps = 1 / time;
	pre = current;
}

float FPS::GetFpsMillis()
{
	return fps;
	//return  60.0f;
}
