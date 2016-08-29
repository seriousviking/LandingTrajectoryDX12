//-----------------------------------------------------------------------------
// D3DManager.cpp
//
// Created by seriousviking at 2016.08.21 22:10
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "D3DManager.h"
#include "Utils/Log.h"
#include "Utils/Macro.h"
#include "Utils/FileUtils.h"

#include "d3dx12.h"

#include <d3dcompiler.h> // to compile shaders in run-time
#include <synchapi.h>// for events

namespace
{
	const size_t DescStringSize = 128;
	const DWORD GPUWaitLimit = 10000; // 10sec

	struct Vertex
	{
		Vertex() {}
		Vertex(float x, float y, float z, float r, float g, float b, float a) : position(x, y, z), color(r, g, b, a) {}

		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
}

D3DManager::D3DManager() :
	_frameBufferCount(3),
	_dedicatedVideoMemorySizeMBs(0),
	_initializationFrame(false),
	_debugController(nullptr),
	_device(nullptr),
	_commandQueue(nullptr),
	_swapChain(nullptr),
	_renderTargetViewHeap(nullptr),
	_bufferIndex(0),
	_commandList(nullptr),
	_fenceEvent(nullptr),
	_pipelineState(nullptr),
	_rootSignature(nullptr),
	_vertexBuffer(nullptr),
	_indexBuffer(nullptr),
	_vertexBufferSize(0),
	_indexBufferSize(0),
	_vertexBufferUploadHeap(nullptr),
	_indexBufferUploadHeap(nullptr),
	_depthStencilBuffer(nullptr),
	_depthStencilDescHeap(nullptr)
{
	// multisampling off
	ZeroMemory(&_sampleDesc, sizeof(_sampleDesc));
	_sampleDesc.Count = 1;
	_sampleDesc.Quality = 0;

}

D3DManager::~D3DManager()
{
	free();
}

bool D3DManager::init(const Def &def)
{
	_def = def;
	if (!createDevice())
	{
		Log(L"createDevice() failed");
		return false;
	}
	if (!createSwapchain())
	{
		Log(L"createSwapchain() failed");
		return false;
	}
	if (!createRenderTargetViews())
	{
		Log(L"createRenderTargetViews() failed");
		return false;
	}
	if (!createCommandList())
	{
		Log(L"createCommandList() failed");
		return false;
	}
	if (!createSyncEvent())
	{
		Log(L"createSyncEvent() failed");
		return false;
	}
	if (!createRootSignature())
	{
		Log(L"createRootSignature() failed");
		return false;
	}
	if (!createPipelineState())
	{
		Log(L"createPipelineState() failed");
		return false;
	}
	if (!createBuffers())
	{
		Log(L"createBuffers() failed");
		return false;
	}
	if (!createDepthStencilBuffers())
	{
		Log(L"createDepthStencilBuffers() failed");
		return false;
	}
	if (!createConstantBuffers())
	{
		Log(L"createConstantBuffers() failed");
		return false;
	}
	if (!finalizeCreatedResources())
	{
		Log(L"finalizeCreatedResources() failed");
		return false;
	}
	prepareViewport();
	prepareCamera();
	_initializationFrame = true;
	return true;
}

void D3DManager::free()
{
	// wait for the gpu to finish the last frame
	// TODO: check if correct
	waitForPreviousFrame();
	//for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	//{
	//	waitForPreviousFrame();
	//}
	if (!closeMappedResources())
	{
		Log(L"Error in closeMappedResources()");
	}
	// get swapchain out of full screen before exiting
	if (_swapChain)
	{
		BOOL isFullScreen = false;
		if (_swapChain->GetFullscreenState(&isFullScreen, NULL))
		{
			_swapChain->SetFullscreenState(false, NULL);
		}
	}
	// close the object handle to the fence event
	auto error = CloseHandle(_fenceEvent);
	if (error == 0)
	{
		Log(L"error closing fence event: %x", error);
	}
	// release upload buffers data
	for (auto cbUploadHeap : _constantBufferUploadHeaps)
	{
		a_SAFE_RELEASE(cbUploadHeap);
	}
	_constantBufferUploadHeaps.clear();
	// release rendering objects
	a_SAFE_RELEASE(_depthStencilBuffer);
	a_SAFE_RELEASE(_depthStencilDescHeap);
	a_SAFE_RELEASE(_vertexBuffer);
	a_SAFE_RELEASE(_indexBuffer);
	a_SAFE_RELEASE(_pipelineState);
	a_SAFE_RELEASE(_rootSignature);
	// release core objects
	for (auto &fence : _fences)
	{
		a_SAFE_RELEASE(fence);
	}
	_fences.clear();
	a_SAFE_RELEASE(_commandList);
	for (auto &commandAllocator : _commandAllocators)
	{
		a_SAFE_RELEASE(commandAllocator);
	}
	_commandAllocators.clear();
	for (auto &renderTarget : _backBufferRenderTargets)
	{
		a_SAFE_RELEASE(renderTarget);
	}
	_backBufferRenderTargets.clear();
	a_SAFE_RELEASE(_renderTargetViewHeap);
	a_SAFE_RELEASE(_swapChain);
	a_SAFE_RELEASE(_commandQueue);

	a_SAFE_RELEASE(_device);
	a_SAFE_RELEASE(_debugController);
}

bool D3DManager::update()
{
	// update app logic, such as moving the camera or figuring out what objects are in view
	using namespace DirectX;
	// create rotation matrices
	XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
	XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
	XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

	XMMATRIX rotMat;
	// add rotation to cube1's rotation matrix and store it
	rotMat = XMLoadFloat4x4(&_cubes[0].rotationMatrix) * rotXMat * rotYMat * rotZMat;
	XMStoreFloat4x4(&_cubes[0].rotationMatrix, rotMat);

	// create translation matrix for cube 1 from cube 1's position vector
	XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&_cubes[0].position));

	// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
	XMMATRIX worldMat = rotMat * translationMat;

	// store cube1's world matrix
	XMStoreFloat4x4(&_cubes[0].worldMatrix, worldMat);

	// update constant buffer for cube1
	// create the wvp matrix and store in constant buffer
	XMMATRIX viewMat = XMLoadFloat4x4(&_cameraViewMatrix); // load view matrix
	XMMATRIX projMat = XMLoadFloat4x4(&_cameraProjectionMatrix); // load projection matrix
	XMMATRIX wvpMat = XMLoadFloat4x4(&_cubes[0].worldMatrix) * viewMat * projMat; // create wvp matrix
	XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&_constantBufferPerObject.wvpMatrix, transposed); // store transposed wvp matrix in constant buffer
	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(_constantBufferPerObjectGPUAddress[_bufferIndex], &_constantBufferPerObject, sizeof(_constantBufferPerObject));

	// now do cube2's world matrix
	// create rotation matrices for cube2
	rotXMat = XMMatrixRotationX(0.0003f);
	rotYMat = XMMatrixRotationY(0.0002f);
	rotZMat = XMMatrixRotationZ(0.0001f);

	// add rotation to cube2's rotation matrix and store it
	rotMat = rotZMat * (XMLoadFloat4x4(&_cubes[1].rotationMatrix) * (rotXMat * rotYMat));
	XMStoreFloat4x4(&_cubes[1].rotationMatrix, rotMat);

	// create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
	XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&_cubes[1].position));

	// we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
	XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);

	// reuse worldMat. 
	// first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
	// then we translate it. 
	// then we rotate it. rotation always rotates around point 0,0,0
	// finally we move it to cube 1's position, which will cause it to rotate around cube 1
	worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

	wvpMat = XMLoadFloat4x4(&_cubes[1].worldMatrix) * viewMat * projMat; // create wvp matrix
	transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
	XMStoreFloat4x4(&_constantBufferPerObject.wvpMatrix, transposed); // store transposed wvp matrix in constant buffer
	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(_constantBufferPerObjectGPUAddress[_bufferIndex] + ConstantBufferPerObjectAlignedSize, 
		&_constantBufferPerObject, sizeof(_constantBufferPerObject));

	// store cube2's world matrix
	XMStoreFloat4x4(&_cubes[1].worldMatrix, worldMat);
	return true;
}

bool D3DManager::render()
{
	HRESULT result;

	if (!updatePipeline())
	{
		Log(L"Update pipeline failed");
		return false;
	}

	// load the command list array (only one command list for now)
	ID3D12CommandList* ppCommandLists[1];
	ppCommandLists[0] = _commandList;
	// execute the list of commands
	_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	// this command goes in at the end of our command queue. We will know when our command queue 
	// has finished because the fence value will be set to "fenceValue" from the GPU since the command
	// queue is being executed on the GPU
	result = _commandQueue->Signal(_fences[_bufferIndex], _fenceValues[_bufferIndex]);
	if (FAILED(result))
	{
		Log(L"Render: command queue Signal() failed: %x. Buffer index: %d", result, _bufferIndex);
		return false;
	}
	// present the back buffer to the screen since rendering is complete
	if (_def.enableVSync)
	{
		// lock to screen refresh rate
		result = _swapChain->Present(1, 0);
		if (FAILED(result))
		{
			Log(L"failed to present swapchain (vsync=On): %x", result);
			return false;
		}
	}
	else
	{
		// present as fast as possible
		result = _swapChain->Present(0, 0);
		if (FAILED(result))
		{
			Log(L"failed to present swapchain (vsync=Off): %x", result);
			return false;
		}
	}

	return true;
}

bool D3DManager::createDevice()
{
	HRESULT result;

#if DEBUG_GRAPHICS_ENABLED
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_debugController))))
	{
		_debugController->EnableDebugLayer();
	}
	else
	{
		Log(L"Failed to get D3D debug interface");
	}
#endif
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
	return true;
}

bool D3DManager::createSwapchain()
{
	HRESULT result;
	// Enumerate monitors and get refresh rate info using DXGI
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

	// enumerate the adapter outputs
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
	// get the number of modes that the selected output supports with format DXGI_FORMAT_R8G8B8A8_UNORM
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
	// get the GPU description
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
	
	// release DXGI interfaces
	a_SAFE_RELEASE(selectedAdapterOutput);
	a_SAFE_RELEASE(dxgiAdapter);
	// prepare swapchain
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	// fill the swap chain description
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = _frameBufferCount;
	// set the size and format of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Width = _def.screenWidth;
	swapChainDesc.BufferDesc.Height = _def.screenHeight;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// discard the previous buffer contents after swapping
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// set the handle for the window to render to
	swapChainDesc.OutputWindow = _def.hwnd;
	// set to full screen or windowed mode
	if (_def.fullScreen)
	{
		swapChainDesc.Windowed = false;
	}
	else
	{
		swapChainDesc.Windowed = true;
	}

	// set the refresh rate of the back buffer depending on VSync flag
	if (_def.enableVSync)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
		Log(L"VSync enabled. Refresh rate: %d/%d (%3.3f)", numerator, denominator, (float)numerator / denominator);
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		Log(L"VSync disabled");
	}
	swapChainDesc.SampleDesc = _sampleDesc;
	if (_sampleDesc.Count == 1 && _sampleDesc.Quality == 0)
	{
		Log(L"Multisampling disabled");
	}
	else
	{
		Log(L"Multisampling enabled: %d / %d", _sampleDesc.Count, _sampleDesc.Quality);
	}
	// scan line ordering and scaling
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// no advanced flags
	swapChainDesc.Flags = 0;

	// create the temporary swap chain
	IDXGISwapChain* swapChain;
	result = dxgiFactory->CreateSwapChain(_commandQueue, &swapChainDesc, &swapChain);
	if (FAILED(result))
	{
		Log(L"Failed to create swapchain: %x", result);
		return false;
	}
	// upgrade the IDXGISwapChain to a IDXGISwapChain3 interface
	// that's needed, for example, to get the current back buffer index
	result = swapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&_swapChain);
	if (FAILED(result))
	{
		Log(L"Failed to create IDXGISwapchain3: %x", result);
		return false;
	}
	// clear pointer to original swap chain interface
	a_SAFE_RELEASE(swapChain);
	// release the factory now that the swap chain has been created
	a_SAFE_RELEASE(dxgiFactory);

	return true;
}

bool D3DManager::createRenderTargetViews()
{
	HRESULT result;

	D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc;
	// initialize the render target view heap description for the two back buffers.
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));
	// set the number of descriptors equal to back buffers count, set the heap type to render target views
	renderTargetViewHeapDesc.NumDescriptors = _frameBufferCount;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	// create the render target view heap for the back buffers
	result = _device->CreateDescriptorHeap(&renderTargetViewHeapDesc,
		__uuidof(ID3D12DescriptorHeap), (void**)&_renderTargetViewHeap);
	if (FAILED(result))
	{
		Log(L"Failed to create descriptor heap: %x", result);
		return false;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	UINT renderTargetViewDescriptorSize;
	// get a handle to the starting memory location in the render target view heap 
	// to identify where the render target views will be located for the back buffers
	renderTargetViewHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	// get the size of the memory location for the render target view descriptors
	renderTargetViewDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// create render target views for all back buffers
	_backBufferRenderTargets.resize(_frameBufferCount, nullptr);
	for (size_t targetIndex = 0; targetIndex < _frameBufferCount; ++targetIndex)
	{
		_backBufferRenderTargets[targetIndex] = nullptr;
	}

	for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	{
		// get a pointer to the back buffer from the swap chain
		result = _swapChain->GetBuffer(bufferIndex, __uuidof(ID3D12Resource),
			(void**)&_backBufferRenderTargets[bufferIndex]);
		if (FAILED(result))
		{
			Log(L"Failed to get back buffer at index %d: %x", bufferIndex, result);
			return false;
		}
		// create a render target view for the current back buffer
		_device->CreateRenderTargetView(_backBufferRenderTargets[bufferIndex], NULL,
			renderTargetViewHandle);
		// increment the view handle to the next descriptor location in the render target view heap
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}
	// get the initial index to which buffer is the current back buffer
	_bufferIndex = _swapChain->GetCurrentBackBufferIndex();
	return true;
}

bool D3DManager::createCommandList()
{
	HRESULT result;
	_commandAllocators.clear();
	_commandAllocators.resize(_frameBufferCount, nullptr);
	// create command allocator for each buffer
	for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	{
		result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			__uuidof(ID3D12CommandAllocator), (void**)&_commandAllocators[bufferIndex]);
		if (FAILED(result))
		{
			Log(L"Failed to create command allocator %d: %x", bufferIndex, result);
			return false;
		}
	}
	// create a simple command list with the first allocator
	result = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		_commandAllocators[0], NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&_commandList);
	if (FAILED(result))
	{
		Log(L"Failed to create command list: %x", result);
		return false;
	}
	// do not close command list yet because we use it to transfer data from CPU to GPU memory
	/*result = _commandList->Close();
	if (FAILED(result))
	{
		Log(L"Failed to close command list: %x", result);
		return false;
	}*/
	return true;
}

bool D3DManager::createSyncEvent()
{
	HRESULT result;
	_fences.clear();
	_fenceValues.clear();
	_fences.resize(_frameBufferCount, nullptr);
	_fenceValues.resize(_frameBufferCount, 0);
	// create a fence for GPU synchronization
	for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	{
		result = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&_fences[bufferIndex]);
		if (FAILED(result))
		{
			return false;
		}
		// initialize the starting fence value
		_fenceValues[bufferIndex] = 0;
	}
	// create an event object for the fence
	// TODO: check is it secure to have name or is it better to leave it as NULL
	_fenceEvent = CreateEventEx(NULL, L"Local\\D3DFenceEvent", FALSE, EVENT_ALL_ACCESS);
	if (_fenceEvent == NULL)
	{
		return false;
	}
	return true;
}

bool D3DManager::createRootSignature()
{
	HRESULT result;

	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;
	// create a root parameter and fill it out
	D3D12_ROOT_PARAMETER  rootParameters[1]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
		rootParameters, // a pointer to the beginning of our root parameters array
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

	ID3DBlob* signatureBlob;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, nullptr);
	if (FAILED(result))
	{
		Log(L"Failed to serialize D3D12 root signature");
		return false;
	}
	result = _device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&_rootSignature));
	if (FAILED(result))
	{
		Log(L"Failed to create D3D12 root signature");
		return false;
	}
	return true;
}

bool D3DManager::createPipelineState()
{
	HRESULT result;

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
#if DEBUG_GRAPHICS_ENABLED
	// compile vertex shader
	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* errorBuff = nullptr;
	result = D3DCompileFromFile(L"../data/shaders/simpleVS.hlsl",
		nullptr,
		nullptr,
		"mainVS",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vertexShader,
		&errorBuff);
	if (FAILED(result))
	{
		if (errorBuff)
		{
			const char* errorString = static_cast<char*>(errorBuff->GetBufferPointer());
			Log("Error compiling vertex shader (%08x): %s", result, errorString);
		}
		else
		{
			Log("Error compiling vertex shader: %08x. No error string created", result);
		}
		return false;
	}
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	ID3DBlob* pixelShader = nullptr;
	result = D3DCompileFromFile(L"../data/shaders/simplePS.hlsl",
		nullptr,
		nullptr,
		"mainPS",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pixelShader,
		&errorBuff);
	if (FAILED(result))
	{
		if (errorBuff)
		{
			const char* errorString = static_cast<char*>(errorBuff->GetBufferPointer());
			Log("Error compiling pixel shader (%08x): %s", result, errorString);
		}
		else
		{
			Log("Error compiling pixel shader: %08x. No error string created", result);
		}
		return false;
	}
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
#else  //DEBUG_GRAPHICS_ENABLED
	const WString simpleVSName(L"simpleVS.cso");
	auto vertexShaderData = Utils::loadFile(simpleVSName);
	if (vertexShaderData.empty())
	{
		Log("Failed to load shader file: %s", simpleVSName.c_str());
		return false;
	}
	vertexShaderBytecode.BytecodeLength = vertexShaderData.size();
	vertexShaderBytecode.pShaderBytecode = vertexShaderData.data();

	const WString simplePSName(L"simplePS.cso");
	auto pixelShaderData = Utils::loadFile(simplePSName);
	if (pixelShaderData.empty())
	{
		Log("Failed to load shader file: %s", simplePSName.c_str());
		return false;
	}
	pixelShaderBytecode.BytecodeLength = pixelShaderData.size();
	pixelShaderBytecode.pShaderBytecode = pixelShaderData.data();
#endif //DEBUG_GRAPHICS_ENABLED

	// create input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// init rasterizer description with default values
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	// init blend description with default values
	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
	{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	for (UINT rtIndex = 0; rtIndex < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++rtIndex)
	{
		blendDesc.RenderTarget[rtIndex] = defaultRenderTargetBlendDesc;
	}
	// create a pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
	psoDesc.pRootSignature = _rootSignature; // the root signature that describes the input data this pso needs
	psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
	psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // format of the depth/stencil buffer
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
	psoDesc.SampleDesc = _sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
	psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
	psoDesc.RasterizerState = rasterizerDesc; // a default rasterizer state
	psoDesc.BlendState = blendDesc; // a default blend state
	psoDesc.NumRenderTargets = 1; // we are only binding one render target
	result = _device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState));
	if (FAILED(result))
	{
		Log(L"Failed to create pipeline state: %08x", result);
		return false;
	}

	return true;
}

bool D3DManager::createBuffers()
{
	HRESULT result;
	// triangle vertices
	Vector<Vertex> vertexList = {
		// front face
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		// right side face
		{ 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		// left side face
		{ -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		// back face
		{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		// top face
		{ -0.5f,  0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
		// bottom face
		{ 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
	};
	Vector<DWORD> indexList = { 
		// front face
		0, 1, 2, 0, 3, 1,
		// left face
		4, 5, 6, 4, 7, 5,
		// right face
		8, 9, 10, 8, 11, 9,
		// back face
		12, 13, 14, 12, 15, 13,
		// top face
		16, 17, 18, 16, 19, 17,
		// bottom face
		20, 21, 22, 20, 23, 21,
	};
	_vertexBufferSize = sizeof(Vertex) * (UINT)vertexList.size();
	_indexBufferSize = sizeof(DWORD) * (UINT)indexList.size();
	_numCubeIndices = (UINT)indexList.size();

	// vertex buffer
	// create default heap
	auto defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	result = _device->CreateCommittedResource(
		&defaultHeapDesc, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(_vertexBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
										// from the upload heap to this heap
		nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&_vertexBuffer));
	if (FAILED(result))
	{
		Log(L"Failed to create vertex buffer: %08x", result);
		return false;
	}
#if DEBUG_GRAPHICS_ENABLED
	_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");
#endif

	// create upload heap
	auto uploadHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	result = _device->CreateCommittedResource(
		&uploadHeapDesc, // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(_vertexBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&_vertexBufferUploadHeap));
	if (FAILED(result))
	{
		Log(L"Failed to create upload vertex buffer: %08x", result);
		return false;
	}

#if DEBUG_GRAPHICS_ENABLED
	_vertexBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");
#endif

	// buffer heaps created, now we need to upload data to GPU
	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = vertexList.data(); // pointer to our vertex array
	vertexData.RowPitch = _vertexBufferSize; // size of all our triangle vertex data
	vertexData.SlicePitch = _vertexBufferSize; // also the size of our triangle vertex data

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	// for simplicity, using the function UpdateSubresources(...) from d3dx12 library
	UpdateSubresources(_commandList, _vertexBuffer, _vertexBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _vertexBuffer;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	_commandList->ResourceBarrier(1, &barrier);

	// index buffer
	// create default heap to hold index buffer
	result = _device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(_indexBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&_indexBuffer));
	if (FAILED(result))
	{
		Log(L"Failed to create index buffer: %08x", result);
		return false;
	}
#if DEBUG_GRAPHICS_ENABLED
	_indexBuffer->SetName(L"Index Buffer Resource Heap");
#endif

	// create upload heap to upload index buffer
	_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(_indexBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&_indexBufferUploadHeap));
	if (FAILED(result))
	{
		Log(L"Failed to create upload index buffer: %08x", result);
		return false;
	}
#if DEBUG_GRAPHICS_ENABLED
	_indexBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");
#endif
	// store index buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = indexList.data(); // pointer to our index array
	indexData.RowPitch = _indexBufferSize; // size of all our index buffer
	indexData.SlicePitch = _indexBufferSize; // also the size of our index buffer
										// we are now creating a command with the command list to copy the data from
										// the upload heap to the default heap
	UpdateSubresources(_commandList, _indexBuffer, _indexBufferUploadHeap, 0, 0, 1, &indexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	auto indexTransitionBarrier =
		CD3DX12_RESOURCE_BARRIER::Transition(_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	_commandList->ResourceBarrier(1, &indexTransitionBarrier);

	return true;
}

bool D3DManager::createDepthStencilBuffers()
{
	HRESULT result;

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	result = _device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _def.screenWidth, _def.screenHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&_depthStencilBuffer)
	);
	if (FAILED(result))
	{
		Log("Failed to create descriptor heap for depth-stencil buffer view: %08x", result);
		return false;
	}
	
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = _device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_depthStencilDescHeap));
	if (FAILED(result))
	{
		Log("Failed to create descriptor heap for depth-stencil buffer view: %08x", result);
		return false;
	}
#if DEBUG_GRAPHICS_ENABLED
	_depthStencilDescHeap->SetName(L"Depth/Stencil Resource Heap");
#endif

	_device->CreateDepthStencilView(_depthStencilBuffer, &depthStencilDesc, 
		_depthStencilDescHeap->GetCPUDescriptorHandleForHeapStart());
	return true;
}

bool D3DManager::createConstantBuffers()
{
	HRESULT result;

	// create the constant buffer resource heap
	// We will update the constant buffer one or more times per frame, so we will use only an upload heap
	// unlike previously we used an upload heap to upload the vertex and index data, and then copied over
	// to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
	// efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
	// will be modified and uploaded at least once per frame, so we only use an upload heap
	_constantBufferUploadHeaps.clear();
	_constantBufferUploadHeaps.resize(_frameBufferCount, nullptr);
	_constantBufferPerObjectGPUAddress.clear();
	_constantBufferPerObjectGPUAddress.resize(_frameBufferCount, nullptr);
	// create a resource heap, descriptor heap, and pointer to cbv for each frame
	for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	{
		// create resource for cube 1
		result = _device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&_constantBufferUploadHeaps[bufferIndex]));
		_constantBufferUploadHeaps[bufferIndex]->SetName(L"Constant Buffer Upload Resource Heap");
		if (FAILED(result))
		{
			Log(L"Failed to create constant buffer %d resource: %08x", bufferIndex, result);
			return false;
		}
		ZeroMemory(&_constantBufferPerObject, sizeof(_constantBufferPerObject));

		// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
		CD3DX12_RANGE readRange(0, 0);
		// map the resource heap to get a gpu virtual address to the beginning of the heap
		result = _constantBufferUploadHeaps[bufferIndex]->Map(0, &readRange, reinterpret_cast<void**>(&_constantBufferPerObjectGPUAddress[bufferIndex]));
		if (FAILED(result))
		{
			Log(L"Failed to map constant buffer %d upload heap: %08x", bufferIndex, result);
			return false;
		}
		// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
		// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
		auto gpuAddress = _constantBufferPerObjectGPUAddress[bufferIndex];
		// cube1's constant buffer data
		memcpy(gpuAddress, &_constantBufferPerObject, sizeof(_constantBufferPerObject));
		// cube2's constant buffer data
		memcpy(gpuAddress + ConstantBufferPerObjectAlignedSize, &_constantBufferPerObject, sizeof(_constantBufferPerObject));
	}

	return true;
}

bool D3DManager::finalizeCreatedResources()
{
	HRESULT result;
	// Now we execute the command list to upload the initial assets (triangle data)
	_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { _commandList };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
	_fenceValues[_bufferIndex]++;
	result = _commandQueue->Signal(_fences[_bufferIndex], _fenceValues[_bufferIndex]);
	if (FAILED(result))
	{
		Log(L"Prepare vertex buffer: command queue Signal() failed: %x. Buffer index: %d", result, _bufferIndex);
		return false;
	}

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	ZeroMemory(&_vertexBufferView, sizeof(_vertexBufferView));
	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.StrideInBytes = sizeof(Vertex);
	_vertexBufferView.SizeInBytes = _vertexBufferSize;
	ZeroMemory(&_indexBufferView, sizeof(_indexBufferView));
	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	_indexBufferView.SizeInBytes = _indexBufferSize;

	return true;
}

void D3DManager::cleanupUploadedResources()
{
	a_SAFE_RELEASE(_vertexBufferUploadHeap);
	a_SAFE_RELEASE(_indexBufferUploadHeap);
}

bool D3DManager::closeMappedResources()
{
	for (UINT bufferIndex = 0; bufferIndex < _frameBufferCount; ++bufferIndex)
	{
		_constantBufferUploadHeaps[bufferIndex]->Unmap(0, nullptr);
	}
	return true;
}

void D3DManager::prepareViewport()
{
	// Fill out the Viewport
	_viewport.TopLeftX = 0;
	_viewport.TopLeftY = 0;
	_viewport.Width = static_cast<FLOAT>(_def.screenWidth);
	_viewport.Height = static_cast<FLOAT>(_def.screenHeight);
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;
	// Fill out a scissor rect
	_scissorRect.left = 0;
	_scissorRect.top = 0;
	_scissorRect.right = _def.screenWidth;
	_scissorRect.bottom = _def.screenHeight;
}

void D3DManager::prepareCamera()
{
	using namespace DirectX;

	XMMATRIX tmpMat;
	XMVECTOR posVec;
	// build projection and view matrix
	Float aspectRatio = (Float)_def.screenWidth / (Float)_def.screenHeight;
	tmpMat = XMMatrixPerspectiveFovLH(45.0f*(3.14f / 180.0f), aspectRatio, 0.1f, 1000.0f);
	XMStoreFloat4x4(&_cameraProjectionMatrix, tmpMat);
	// set starting camera state
	_cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
	_cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	_cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	// build view matrix
	XMVECTOR cPos = XMLoadFloat4(&_cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&_cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&_cameraUp);
	tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&_cameraViewMatrix, tmpMat);
	// set starting cubes position
	_cubes.clear();
	_cubes.resize(2);
	// first cube
	_cubes[0].position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	posVec = XMLoadFloat4(&_cubes[0].position);
	tmpMat = XMMatrixTranslationFromVector(posVec);
	XMStoreFloat4x4(&_cubes[0].rotationMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&_cubes[0].worldMatrix, tmpMat);
	// second cube
	_cubes[1].position = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
	posVec = XMLoadFloat4(&_cubes[1].position) + XMLoadFloat4(&_cubes[0].position);
	tmpMat = XMMatrixTranslationFromVector(posVec);
	XMStoreFloat4x4(&_cubes[1].rotationMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&_cubes[1].worldMatrix, tmpMat);
}

bool D3DManager::updatePipeline()
{
	HRESULT result;
	// we have to wait for the gpu to finish with the command allocator before we reset it
	if (!waitForPreviousFrame())
	{
		Log(L"waitForPreviousFrame() failed");
		return false;
	}
	if (_initializationFrame)
	{
		cleanupUploadedResources();
		_initializationFrame = false;
	}
	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	result = _commandAllocators[_bufferIndex]->Reset();
	if (FAILED(result))
	{
		Log(L"Failed to reset command allocator %d: %08x", _bufferIndex, result);
		return false;
	}

	// Reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// The command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter,
	// but in this tutorial we are only clearing the rtv, and do not actually need
	// anything but an initial default pipeline, which is what we get by setting
	// the second parameter to NULL
	result = _commandList->Reset(_commandAllocators[_bufferIndex], _pipelineState);
	if (FAILED(result))
	{
		Log(L"Failed to reset command list %d: %08x", _bufferIndex, result);
		return false;
	}

	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

	// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	D3D12_RESOURCE_BARRIER barrier;
	// set the resource barrier
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = _backBufferRenderTargets[_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	_commandList->ResourceBarrier(1, &barrier);

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	auto renderTargetViewHandle = _renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	auto renderTargetViewDescriptorSize =
		_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	renderTargetViewHandle.ptr += _bufferIndex * renderTargetViewDescriptorSize;
	// get a handle to the depth/stencil buffer
	auto depthStencilViewHandle = _depthStencilDescHeap->GetCPUDescriptorHandleForHeapStart();

	// set the render target for the output merger stage (the output of the pipeline)
	_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, &depthStencilViewHandle);

	// Clear the render target color and depth/stencil
	const float clearColor[] = { 0.3f, 0.3f, 0.8f, 1.0f };
	_commandList->ClearRenderTargetView(renderTargetViewHandle, clearColor, 0, nullptr);
	_commandList->ClearDepthStencilView(depthStencilViewHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	
	_commandList->SetGraphicsRootSignature(_rootSignature);

	_commandList->RSSetViewports(1, &_viewport); // set the viewports
	_commandList->RSSetScissorRects(1, &_scissorRect); // set the scissor rects
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
	_commandList->IASetVertexBuffers(0, 1, &_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
	_commandList->IASetIndexBuffer(&_indexBufferView);
	
	// first cube
	// set cube1's constant buffer
	_commandList->SetGraphicsRootConstantBufferView(0, _constantBufferUploadHeaps[_bufferIndex]->GetGPUVirtualAddress());
	// draw first cube
	_commandList->DrawIndexedInstanced(_numCubeIndices, 1, 0, 0, 0);

	// second cube
	// set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
	// resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
	// cube2's constant buffer data is stored after (256 bits from the start of the heap).
	_commandList->SetGraphicsRootConstantBufferView(0, _constantBufferUploadHeaps[_bufferIndex]->GetGPUVirtualAddress() + ConstantBufferPerObjectAlignedSize);
	// draw second cube
	_commandList->DrawIndexedInstanced(_numCubeIndices, 1, 0, 0, 0);

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	_commandList->ResourceBarrier(1, &barrier);

	result = _commandList->Close();
	if (FAILED(result))
	{
		Log(L"Failed to close command list %d: %08x", _bufferIndex, result);
		return false;
	}

	return true;
}

bool D3DManager::waitForPreviousFrame()
{
	HRESULT result;

	// swap the current rtv buffer index so we draw on the correct buffer
	_bufferIndex = _swapChain->GetCurrentBackBufferIndex();

	// if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
	// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (_fences[_bufferIndex]->GetCompletedValue() < _fenceValues[_bufferIndex])
	{
		// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
		result = _fences[_bufferIndex]->SetEventOnCompletion(_fenceValues[_bufferIndex], _fenceEvent);
		if (FAILED(result))
		{
			Log(L"fences[%d]->SetEventOnCompletion() failed: %x", _bufferIndex, result);
			return false;
		}

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		DWORD waitResult = WaitForSingleObject(_fenceEvent, GPUWaitLimit);
		if (waitResult != WAIT_OBJECT_0)
		{
			Log(L"WaitForSingleObject() failed: %08x. Buffer index: %d", waitResult, _bufferIndex);
			return false;
		}
	}

	// increment fenceValue for next frame
	_fenceValues[_bufferIndex]++;

	return true;
}

