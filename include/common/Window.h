#pragma once
#include <Windows.h>

class WindowBase
{
	HWND hwnd;
	WNDCLASSEX wndClass;
	char bkColor[3];
public:
	~WindowBase();
	void Create(const wchar_t* name,int width, int height, WNDPROC wndproc, char* bkColor = nullptr);
	HWND GetHWND();
	bool ProcessMessage();
};
