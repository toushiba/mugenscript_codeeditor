#include "Input.h"

void KeyBord::Update()
{
	GetKeyboardState(key);
	for (int i = 0; i < 256; i++)
	{
		upFlag[i] = false;
		if (key[i] & STATE_PUSH_BUTTON)
			states[i]++;
		else
		{
			if (states[i])
				upFlag[i] = true;
			states[i] = 0;
		}
	}
}

bool KeyBord::IsPushKey(char key)
{
	return states[key];
}

bool KeyBord::IsDownKey(char key)
{
	return states[key] == 1;
}

bool KeyBord::IsUpKey(char key)
{
	return upFlag[key];
}

void Mouse::Update()
{
	leftButtonUpFlag = false;
	rightButtonUpFlag = false;
	POINT point = {};
	
	GetCursorPos(&point);
	moveVector.x = point.x - cursorPos.x;
	moveVector.y = point.y - cursorPos.y;
	cursorPos = point;
	
	if (GetKeyState(VK_LBUTTON) & STATE_PUSH_BUTTON)
		leftButton++;
	else
	{
		if (leftButton)
			leftButtonUpFlag = true;
		leftButton = 0;
	}
	if (GetKeyState(VK_RBUTTON) & STATE_PUSH_BUTTON)
		rightButton++;
	else
	{
		if (rightButton)
			rightButtonUpFlag = true;
		rightButton = 0;
	}
}

bool Mouse::IsPushLeftButton()
{
	return leftButton;
}

bool Mouse::IsPushRightButton()
{
	return rightButton;
}

bool Mouse::IsDownLeftButton()
{
	return leftButton==1;
}

bool Mouse::IsDownRightButton()
{
	return rightButton==1;
}

bool Mouse::IsUpLeftButton()
{
	return leftButtonUpFlag;
}

bool Mouse::IsUpRightButton()
{
	return rightButtonUpFlag;
}

POINT Mouse::GetMousePos()
{
	return cursorPos;
}

POINT Mouse::GetMouseMoveVector()
{
	return moveVector;
}

void Mouse::SetMouseCursorPos(POINT point)
{
	SetCursorPos(point.x, point.y);
	cursorPos = point;
	moveVector = POINT{ 0,0 };
}



KeyBord g_keybord;
Mouse g_mouse;