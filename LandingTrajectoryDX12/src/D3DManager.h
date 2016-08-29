//-----------------------------------------------------------------------------
// D3DManager.h
//
// Created by seriousviking at 2016.08.21 22:10
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// sources:
// https://digitalerr0r.wordpress.com/2015/08/19/quickstart-directx-12-programming/
// http://www.rastertek.com/dx12tut03.html
// https://software.intel.com/en-us/articles/tutorial-migrating-your-apps-to-directx-12-part-3
// http://www.braynzarsoft.net/viewtutorial/q16390-04-directx-12-braynzar-soft-tutorials
#pragma once
#include <d3d12.h>
#include <dxgi1_5.h>
#include <windef.h> // for HWND dependency
#include <wrl/client.h> // for Microsoft::WRL::ComPtr

#include "Inc/DirectXMath.h"

#include "GraphicsCommon.h"
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
	bool update();
	bool render();

//private:
//	bool createDeviceDependentResources();
//	bool createWindowSizeDependentResources();

private:
	// initialization methods
	bool createDevice();
	bool createSwapchain();
	bool createRenderTargetViews();
	bool createCommandList();
	bool createSyncEvent();

	bool createRootSignature();
	bool createPipelineState();
	bool createBuffers();
	bool createDepthStencilBuffers();
	bool createConstantBuffers();
	bool finalizeCreatedResources();
	void cleanupUploadedResources();
	bool closeMappedResources();
	void prepareViewport();
	// update/draw methods
	bool updatePipeline();
	bool waitForPreviousFrame();

private:
	Def _def;
	UINT _frameBufferCount;
	WString _videoCardDescription;
	uInt _dedicatedVideoMemorySizeMBs;
	bool _initializationFrame;

#if DEBUG_GRAPHICS_ENABLED
	ID3D12Debug* _debugController;
#endif
	ID3D12Device* _device; // TODO: move to ComPtr<>
	ID3D12CommandQueue* _commandQueue;
	IDXGISwapChain3* _swapChain;
	ID3D12DescriptorHeap* _renderTargetViewHeap;
	unsigned int _bufferIndex;
	ID3D12GraphicsCommandList* _commandList;
	DXGI_SAMPLE_DESC _sampleDesc;

	HANDLE _fenceEvent;

	Vector<ID3D12Resource*> _backBufferRenderTargets;
	Vector<ID3D12CommandAllocator*> _commandAllocators;
	Vector<ID3D12Fence*> _fences;
	Vector<UINT64> _fenceValues;
	//pipeline
	ID3D12PipelineState* _pipelineState;
	ID3D12RootSignature* _rootSignature;
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _scissorRect;
	//vertex/index buffers
	UINT _vertexBufferSize;
	UINT _indexBufferSize;
	ID3D12Resource* _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	ID3D12Resource* _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;
	ID3D12Resource* _vertexBufferUploadHeap;
	ID3D12Resource* _indexBufferUploadHeap;

	//Depth/Stencil
	ID3D12Resource* _depthStencilBuffer;
	ID3D12DescriptorHeap* _depthStencilDescHeap;
	//Constant buffer
	struct ConstantBufferData
	{
		DirectX::XMFLOAT4 colorMultiplier;
	};
	Vector<ID3D12DescriptorHeap*> _mainCBDescriptorHeaps;
	Vector<ID3D12Resource*> _constantBufferUploadHeaps;
	ConstantBufferData _colorMultiplierData;
	Vector<void*> _cbColorMultiplierGPUAddress;

};
