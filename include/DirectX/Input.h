#pragma once
#include <Windows.h>

#define STATE_PUSH_BUTTON 0x80

class KeyBord
{
	BYTE key[256];
	BYTE upFlag[256];
	int states[256];
public:

	void Update();
	bool IsPushKey(char key);
	bool IsDownKey(char key);
	bool IsUpKey(char key);
};

class Mouse
{
public:

	void Update();
	bool IsPushLeftButton();
	bool IsPushRightButton();
	bool IsDownLeftButton();
	bool IsDownRightButton();
	bool IsUpLeftButton();
	bool IsUpRightButton();
	POINT GetMousePos();
	POINT GetMouseMoveVector();
	void SetMouseCursorPos(POINT);

private:

	POINT cursorPos = {};
	POINT moveVector = {};
	int leftButton = 0;
	int rightButton = 0;
	bool leftButtonUpFlag = false;
	bool rightButtonUpFlag = false;
};

extern KeyBord g_keybord;
extern Mouse g_mouse;