//-----------------------------------------------------------------------------
// GraphicsManager.cpp
//
// Created by seriousviking at 2016.08.21 18:22
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "GraphicsManager.h"
#include "Application.h"

GraphicsManager::GraphicsManager()
{

}

GraphicsManager::~GraphicsManager()
{
	free();
}

bool GraphicsManager::init(const WindowDef &windowDef)
{
	_d3dManager.reset(new D3DManager);
	D3DManager::Def d3dDef;
	d3dDef.enableVSync = false;
	d3dDef.fullScreen = windowDef.fullScreen;
	d3dDef.screenWidth = windowDef.screenWidth;
	d3dDef.screenHeight = windowDef.screenHeight;
	d3dDef.hwnd = windowDef.hwnd;
	if (!_d3dManager->init(d3dDef))
	{
		return false;
	}
	return true;
}

void GraphicsManager::free()
{
	_d3dManager.reset();
}

bool GraphicsManager::processFrame()
{
	if (!render())
	{
		return false;
	}
	return true;
}

bool GraphicsManager::render()
{
	assert(_d3dManager);
	if (!_d3dManager->render())
	{
		return false;
	}
	return true;
}
