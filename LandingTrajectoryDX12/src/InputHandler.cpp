//-----------------------------------------------------------------------------
// InputHandler.cpp
//
// Created by seriousviking at 2016.08.21 18:17
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "InputHandler.h"
#include "Application.h"

InputHandler::InputHandler()
{
}

InputHandler::~InputHandler()
{
}

bool InputHandler::init()
{
	ZeroMemory(_keys, sizeof(_keys));
	return true;
}

void InputHandler::keyDown(unsigned int input)
{
	_keys[input] = true;
}

void InputHandler::keyUp(unsigned int input)
{
	_keys[input] = false;
}

bool InputHandler::isKeyDown(unsigned int key)
{
	return _keys[key];
}
