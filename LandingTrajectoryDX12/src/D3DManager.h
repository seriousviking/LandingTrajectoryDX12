//-----------------------------------------------------------------------------
// D3DManager.h
//
// Created by seriousviking at 2016.08.21 22:10
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once
#include <d3d12.h>
#include <dxgi1_5.h>

#include "Application.h" // for HWND dependency
#include "Utils/Types.h"

class D3DManager
{
public:
	struct Def
	{
		int screenWidth;
		int screenHeight; 
		bool enableVSync;
		HWND hwnd;
		bool fullScreen;
	};

	D3DManager();
	D3DManager(const D3DManager& other) = delete;
	~D3DManager();

	bool init(const Def &def);
	void free();
	bool render();

private:
	Def _def;
	WString _videoCardDescription;
	uInt _dedicatedVideoMemorySizeMBs;

	ID3D12Device* _device;
	ID3D12CommandQueue* _commandQueue;
	IDXGISwapChain3* _swapChain;
	ID3D12DescriptorHeap* _renderTargetViewHeap;
	Vector<ID3D12Resource*> _backBufferRenderTargets;
	unsigned int _bufferIndex;
	ID3D12CommandAllocator* _commandAllocator;
	ID3D12GraphicsCommandList* _commandList;
	ID3D12PipelineState* _pipelineState;
	ID3D12Fence* _fence;
	HANDLE _fenceEvent;
	unsigned long long _fenceValue;
};
