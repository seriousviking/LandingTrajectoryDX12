//-----------------------------------------------------------------------------
// InputHandler.h
//
// Created by seriousviking at 2016.08.21 18:17
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once

class InputHandler
{
public:
	InputHandler();
	InputHandler(const InputHandler& other) = delete;
	~InputHandler();

	bool init();

	void keyDown(unsigned int key);
	void keyUp(unsigned int key);

	bool isKeyDown(unsigned int key);

private:
	bool _keys[256];
};
