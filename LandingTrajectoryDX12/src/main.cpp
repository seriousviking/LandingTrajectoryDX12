//-----------------------------------------------------------------------------
// main.cpp
//
// Created by seriousviking at 2016.08.21 16:11
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "Application.h"
#include "Types/Types.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
	SharedPtr<Application> application;
	bool result;

	application.reset(new Application());

	result = application->init(false);
	if (result)
	{
		application->run();
	}

	application.reset();
	return 0;
}
