#include "common/Window.h"

WindowBase::~WindowBase()
{
	UnregisterClass(wndClass.lpszClassName, wndClass.hInstance);
}

void WindowBase::Create(const wchar_t* name, int width, int height, WNDPROC wndproc, char* bkColor)
{
	wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = (WNDPROC)wndproc;
	wndClass.lpszClassName = (name);
	wndClass.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&wndClass);
	RECT wrc = { 0,0,width,height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	hwnd = CreateWindow(wndClass.lpszClassName,
		name,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wndClass.hInstance,
		nullptr
	);

	if (bkColor)
		for (int i = 0; i < 3; ++i)
			this->bkColor[i] = bkColor[i];
	else
		for (int i = 0; i < 3; ++i)
			this->bkColor[i] = 0;
	ShowWindow(hwnd, SW_SHOW);
	auto dc = GetDC(hwnd);
	SetBkColor(dc, RGB(this->bkColor[0], this->bkColor[1], this->bkColor[2]));
	ReleaseDC(hwnd, dc);
}

HWND WindowBase::GetHWND()
{
	return hwnd;
}

bool WindowBase::ProcessMessage()
{
	MSG msg = {};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT)
	{
		return false;
	}
	return true;
}
