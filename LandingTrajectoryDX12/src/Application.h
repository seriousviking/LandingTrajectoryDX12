//-----------------------------------------------------------------------------
// Application.h
//
// Created by seriousviking at 2016.08.21 16:11
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// class Application is based on ideas from http://www.rastertek.com/tutdx12.html
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Utils/Types.h"
#include "InputHandler.h"
#include "GraphicsManager.h"

struct WindowDef
{
	int screenWidth;
	int screenHeight;
	bool fullScreen;
	bool hideCursor;
	HWND hwnd;
};

class Application
{
public:
	Application();
	Application(const Application& other) = delete;
	~Application();

	bool init(bool fullScreen);
	void free();
	void run();

	LRESULT CALLBACK messageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	bool processFrame();
	WindowDef initializeWindows(bool fullScreen, bool hideCursor);
	void shutdownWindows();

private:
	UniquePtr<InputHandler> _input;
	UniquePtr<GraphicsManager> _graphics;

	WindowDef _windowDef;
	LPCWSTR _applicationName;
	HINSTANCE _hinstance;
	HWND _hwnd;
};
