//-----------------------------------------------------------------------------
// Application.cpp
//
// Created by seriousviking at 2016.08.21 16:11
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "Application.h"

// global app pointer to accept Windows messages. It's not convenient to use 
// shared pointers here
static Application* g_applicationPtr;

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	assert(g_applicationPtr);
	switch (umessage)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return g_applicationPtr->messageHandler(hwnd, umessage, wparam, lparam);
		}
	}
}

Application::Application() : 
	_input(nullptr),
	_graphics(nullptr),
	_applicationName(L"Landing Trajectory DX12"),
	_hinstance(nullptr),
	_hwnd(nullptr)
{
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

Application::~Application()
{
	free();
}

bool Application::init(bool fullScreen)
{
	_windowDef = initializeWindows(fullScreen, false);

	_input.reset(new InputHandler);
	if (!_input->init())
	{
		return false;
	}
	_graphics.reset(new GraphicsManager);
	if (!_graphics->init(_windowDef))
	{
		return false;
	}
	return true;
}

void Application::free()
{
	if (_graphics)
	{
		_graphics.reset();
	}
	if (_input)
	{
		_input.reset();
	}
	shutdownWindows();
}

void Application::run()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	bool done = false;
	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			bool result = processFrame();
			if (!result)
			{
				done = true;
			}
		}

	}
}

LRESULT CALLBACK Application::messageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		case WM_KEYDOWN:
		{
			_input->keyDown((unsigned int)wparam);
			return 0;
		}
		case WM_KEYUP:
		{
			_input->keyUp((unsigned int)wparam);
			return 0;
		}
		default:
		{
			return DefWindowProc(hwnd, umsg, wparam, lparam);
		}
	}
	assert(false); // return from default: label
	return 0;
}

bool Application::processFrame()
{
	assert(_input.get());
	if (_input->isKeyDown(VK_ESCAPE))
	{
		return false;
	}
	bool result = _graphics->processFrame();
	if (!result)
	{
		return false;
	}
	return true;
}

WindowDef Application::initializeWindows(bool fullScreen, bool hideCursor)
{
	WindowDef result;
	result.fullScreen = fullScreen;
	result.hideCursor = hideCursor;

	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	// init global pointer to handle Windows messages
	g_applicationPtr = this;

	_hinstance = GetModuleHandle(NULL);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = _hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = _applicationName;
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	result.screenHeight = GetSystemMetrics(SM_CYSCREEN);
	result.screenWidth = GetSystemMetrics(SM_CXSCREEN);

	if (fullScreen)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsHeight = (unsigned long)result.screenHeight;
		dmScreenSettings.dmPelsWidth = (unsigned long)result.screenWidth;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
		posX = posY = 0;
	}
	else
	{
		result.screenWidth = 1280;
		result.screenHeight = 720;

		posX = (GetSystemMetrics(SM_CXSCREEN) - result.screenWidth) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - result.screenHeight) / 2;
	}

	_hwnd = CreateWindowEx(WS_EX_APPWINDOW, _applicationName, _applicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, result.screenWidth, result.screenHeight, NULL, NULL, _hinstance, NULL);
	result.hwnd = _hwnd;

	ShowWindow(_hwnd, SW_SHOW);
	SetForegroundWindow(_hwnd);
	SetFocus(_hwnd);

	if (hideCursor)
	{
		ShowCursor(false);
	}
	return result;
}

void Application::shutdownWindows()
{
	if (_windowDef.hideCursor)
	{
		ShowCursor(true);
	}
	if (_windowDef.fullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}
	if (_hwnd)
	{
		DestroyWindow(_hwnd);
		_hwnd = nullptr;
		_windowDef.hwnd = nullptr;
	}
	if (_hinstance)
	{
		UnregisterClass(_applicationName, _hinstance);
		_hinstance = nullptr;
	}
	g_applicationPtr = nullptr;
}
