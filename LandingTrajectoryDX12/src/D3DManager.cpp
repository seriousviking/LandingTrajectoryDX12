//-----------------------------------------------------------------------------
// D3DManager.cpp
//
// Created by seriousviking at 2016.08.21 22:10
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "D3DManager.h"

namespace
{
	const size_t DescStringSize = 128;
	const size_t TotalBufferTargets = 2;
}

D3DManager::D3DManager() :
	_device(nullptr),
	_commandQueue(nullptr),
	_swapChain(nullptr),
	_renderTargetViewHeap(nullptr),
	_bufferIndex(0),
	_commandAllocator(nullptr),
	_commandList(nullptr),
	_pipelineState(nullptr),
	_fence(nullptr),
	_fenceEvent(nullptr),
	_fenceValue(0)
{
	_videoCardDescription.resize(DescStringSize);
	_backBufferRenderTargets.resize(TotalBufferTargets);
	for (size_t targetIndex = 0; targetIndex < TotalBufferTargets; ++targetIndex)
	{
		_backBufferRenderTargets[targetIndex] = nullptr;
	}
}

D3DManager::~D3DManager()
{
	free();
}

bool D3DManager::init(const Def &def)
{
	_def = def;
}

void D3DManager::free()
{

}

bool D3DManager::render()
{

}
