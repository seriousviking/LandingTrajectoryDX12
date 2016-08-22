//-----------------------------------------------------------------------------
// D3DManager.cpp
//
// Created by seriousviking at 2016.08.21 22:10
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "D3DManager.h"
#include "Utils/Log.h"

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
	ZeroMemory(_videoCardDescription.data(), DescStringSize * sizeof(char));
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

	HRESULT result;

	// create device with specified feature level
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
	// TODO: select adapter here
	result = D3D12CreateDevice(NULL, featureLevel, __uuidof(ID3D12Device), (void**)&_device);
	if (FAILED(result))
	{
		Log(L"Failed to create DirectX 12.0 device: %x", result);
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	ZeroMemory(&commandQueueDesc, sizeof(commandQueueDesc));
	// Set up the description of the command queue.
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.NodeMask = 0;
	result = _device->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&_commandQueue);
	if (FAILED(result))
	{
		Log(L"Failed to create command queue: %x", result);
		return false;
	}

	// Enumerate monitors and get refresh rate info using DXGI
	// TODO: why Factory5 doesn't work?
	IDXGIFactory4 *dxgiFactory = nullptr;
	result = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&dxgiFactory);
	if (FAILED(result))
	{
		Log(L"Failed to create DXGI factory: %x", result);
		return false;
	}
	// create an adapter for the primary graphics card
	// TODO: select from all available GPUs
	// TODO: distinguish between different structure versions (Adapter, Adapter1,...)
	IDXGIAdapter1 *dxgiAdapter = nullptr;
	result = dxgiFactory->EnumAdapters1(0, &dxgiAdapter);
	if (FAILED(result))
	{
		Log(L"Failed at dxgiFactory->EnumAdapters1(): %x", result);
		return false;
	}

	// TODO: get video and system memory from desc
	//DXGI_ADAPTER_DESC adapterDesc;
	//dxgiAdapter->GetDesc(&adapterDesc);
	// Enumerate the primary adapter output (monitor).
	//IDXGIOutput* dxgiAdapterOutput;
	//result = dxgiAdapter->EnumOutputs(0, &dxgiAdapterOutput);
	//if (FAILED(result))
	//{
	//	return false;
	//}

	IDXGIOutput* dxgiOutput = nullptr;
	IDXGIOutput* selectedAdapterOutput = nullptr;
	DXGI_OUTPUT_DESC dxgiOutDesc;
	Log(L"Enumerating outputs from default adapter");
	for (UINT outputIndex = 0; dxgiAdapter->EnumOutputs(outputIndex, &dxgiOutput) != DXGI_ERROR_NOT_FOUND; ++outputIndex)
	{
		result = dxgiOutput->GetDesc(&dxgiOutDesc);
		if (SUCCEEDED(result))
		{
			Log(L"Output %d: Name: \"%ls\" DesktopCoordinates: (%d %d %d %d)", outputIndex, dxgiOutDesc.DeviceName, 
				dxgiOutDesc.DesktopCoordinates.left, dxgiOutDesc.DesktopCoordinates.top, 
				dxgiOutDesc.DesktopCoordinates.right, dxgiOutDesc.DesktopCoordinates.bottom);
		}
		// TODO: provide user selection of monitor
		if (outputIndex == 0)
		{
			selectedAdapterOutput = dxgiOutput;
		}
		else
		{
			dxgiOutput->Release();
			dxgiOutput = nullptr;
		}
	}
	if (!selectedAdapterOutput)
	{
		Log(L"Appropriate adapter output not found");
		return false;
	}

	UINT modeCount = 0;
	// Get the number of modes that the selected output supports with format DXGI_FORMAT_R8G8B8A8_UNORM
	result = selectedAdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modeCount, NULL);
	if (FAILED(result))
	{
		Log(L"Failed to get display mode list count: %x", result);
		return false;
	}

	// fill display mode list
	Vector<DXGI_MODE_DESC> displayModeList(modeCount);
	result = selectedAdapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modeCount, displayModeList.data());
	if (FAILED(result))
	{
		Log(L"Failed to retrieve display modes: %x", result);
		return false;
	}

	UINT numerator = 0;
	UINT denominator = 0;
	// Now go through all the display modes and find the one that matches the screen height and width.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	for (UINT modeIndex = 0; modeIndex < modeCount; modeIndex++)
	{
		if (displayModeList[modeIndex].Height == (unsigned int)_def.screenHeight)
		{
			if (displayModeList[modeIndex].Width == (unsigned int)_def.screenWidth)
			{
				numerator = displayModeList[modeIndex].RefreshRate.Numerator;
				denominator = displayModeList[modeIndex].RefreshRate.Denominator;
			}
		}
	}

	// Get the GPU description.
	DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
	result = dxgiAdapter->GetDesc1(&dxgiAdapterDesc);
	if (FAILED(result))
	{
		Log(L"Failed to get adapter description: %x", result);
		return false;
	}

	// compute the dedicated video card memory size in megabytes, also save adapter's text description
	_dedicatedVideoMemorySizeMBs = (uInt)(dxgiAdapterDesc.DedicatedVideoMemory / 1024 / 1024);
	_videoCardDescription.assign(dxgiAdapterDesc.Description);
	Log(L"Selected adapter: '%ls' Memory: %d MB", _videoCardDescription.c_str(), _dedicatedVideoMemorySizeMBs);

	// Release DXGI interfaces
	selectedAdapterOutput->Release();
	selectedAdapterOutput = nullptr;
	dxgiAdapter->Release();
	dxgiAdapter = nullptr;

	// Prepare swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 2; // 2 buffers should be enough for now
	// Set the size and format of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Width = _def.screenWidth;
	swapChainDesc.BufferDesc.Height = _def.screenHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Discard the previous buffer contents after swapping
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = _def.hwnd;
	// Set to full screen or windowed mode.
	if (_def.fullScreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	//IDXGISwapChain* swapChain;
	//D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	//D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
}

void D3DManager::free()
{

}

bool D3DManager::render()
{

}
