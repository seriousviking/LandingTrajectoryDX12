//-----------------------------------------------------------------------------
// GraphicsManager.h
//
// Created by seriousviking at 2016.08.21 18:22
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once
#include "D3DManager.h"

struct WindowDef;
// class GraphicsManager is a wrapper around manager which handles all Direct3D work
class GraphicsManager
{
public:
	GraphicsManager();
	~GraphicsManager();

	bool init(const WindowDef &windowDef);
	void free();
	bool processFrame();

private:
	bool update();
	bool render();

private:
	UniquePtr<D3DManager> _d3dManager;
};
