#include "Graphics.h"
#include "system.h"
Graphics graphics;
Graphics2 graphics2;

void Graphics::ActiveDebugLayer()
{
#ifdef _DEBUG
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->Release();
	}
#endif

}


HRESULT Graphics::CreateCmdQueue()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	return _device->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));
}

HRESULT Graphics::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Height = win_height;
	swapChainDesc.Width = win_width;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferCount = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	auto result = factory->CreateSwapChainForHwnd(
		cmdQueue,
		window.GetHWND(),
		&swapChainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&swapchain
	);

	ResultErrorMessage(result, L"スワップチェーンの生成に失敗しました");


	return result;
}

HRESULT Graphics::CreateDescriptorHeapFrameBuffer()
{
	rtvHeap = CreateDescriptorHeap(2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	dsvHeap = CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dsvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < 2; i++)
	{
		swapchain->GetBuffer(i, IID_PPV_ARGS(&rtvFrameBuffer[i]));
		_device->CreateRenderTargetView(rtvFrameBuffer[i], nullptr, rtvHandle);
		rtvHandle.ptr += rtvDescriptorSize;
	}

	rtvFrameBackBufferIdx = swapchain->GetCurrentBackBufferIndex();

	D3D12_CLEAR_VALUE dsvClearValue = {};
	dsvClearValue.Format = dsvFormat;
	dsvClearValue.DepthStencil.Depth = 1.0f;
	dsvClearValue.DepthStencil.Stencil = 0;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format =dsvFormat;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	desc.Width = win_width;
	desc.Height = win_height;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES prop = {};
	prop.Type = D3D12_HEAP_TYPE_DEFAULT;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	auto result = _device->CreateCommittedResource(
		&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsvClearValue, IID_PPV_ARGS(&dsvFrameBuffer)
	);

	ResultErrorMessage(result, L"デプスステンシルバッファの生成に失敗しました");
	_device->CreateDepthStencilView(
		dsvFrameBuffer, nullptr, dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);


	return result;
}

Graphics::~Graphics()
{
	ReleaseResource();
	//if (cmdAlloc)
	//{
	//	cmdAlloc->Release();
	//	cmdAlloc = nullptr;
	//}
	//if (cmdList)
	//{
	//	cmdList->Release();
	//	cmdList = nullptr;
	//}
	//if (cmdQueue)
	//{
	//	cmdQueue->Release();
	//	cmdQueue = nullptr;
	//}
	//for(int i=0;i<2;++i)
	//	if (rtvFrameBuffer[i])
	//	{
	//		rtvFrameBuffer[i]->Release();
	//		rtvFrameBuffer[i] = nullptr;
	//	}
	//if (dsvFrameBuffer)
	//{
	//	dsvFrameBuffer->Release();
	//	dsvFrameBuffer = nullptr;
	//}
	//if (rtvHeap)
	//{
	//	rtvHeap->Release();
	//	rtvHeap = nullptr;
	//}
	//if (dsvHeap)
	//{
	//	dsvHeap->Release();
	//	dsvHeap = nullptr;
	//}
	//if (heapForImgui)
	//{
	//	heapForImgui->Release();
	//	heapForImgui = nullptr;
	//}
	//if (pipelineState)
	//{
	//	pipelineState->Release();
	//	pipelineState = nullptr;
	//}
	//if (factory)
	//{
	//	factory->Release();
	//	factory = nullptr;
	//}
	//if (swapchain)
	//{
	//	swapchain->Release();
	//	swapchain = nullptr;
	//}
	//if (fence)
	//{
	//	fence->Release();
	//	fence = nullptr;
	//}
	//if (_device)
	//{
	//	_device->Release();
	//	_device = nullptr;
	//}
	
}

void Graphics::Init()
{
	EnableDebugLayer();
	window.Create(win_width, win_height);
	auto result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&_device));
	ResultErrorMessage(result, L"デバイスの生成に失敗しました");
	result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	ResultErrorMessage(result, L"ファクトリの生成に失敗しました");
	result = _device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAlloc));
	ResultErrorMessage(result, L"アロケータの生成に失敗しました");
	result = _device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAlloc, nullptr, IID_PPV_ARGS(&cmdList));
	ResultErrorMessage(result, L"コマンドリストの生成に失敗しました");
	result = CreateCmdQueue();
	ResultErrorMessage(result, L"コマンドキューの生成に失敗しました");
	result = CreateSwapChain();
	ResultErrorMessage(result, L"スワップチェインの生成に失敗しました");
	result = _device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	ResultErrorMessage(result, L"フェンスの生成に失敗しました");
	CreateDescriptorHeapFrameBuffer();
	viewport.Width = win_width;
	viewport.Height = win_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = scissor.left + win_width;
	scissor.bottom = scissor.top + win_height;
}

ID3D12Device5* Graphics::GetDevice()
{
	return _device;
}

ID3D12CommandAllocator* Graphics::GetCmdAlloc()
{
	return cmdAlloc;
}

ID3D12GraphicsCommandList4* Graphics::GetCmdList()
{
	return cmdList;
}

ID3D12CommandQueue* Graphics::GetCmdQueue()
{
	return cmdQueue;
}

IDXGIFactory4* Graphics::GetFactory()
{
	return factory;
}

IDXGISwapChain3* Graphics::GetSwapchain()
{
	return swapchain;
}

D3D12_CPU_DESCRIPTOR_HANDLE Graphics::GetRtvFrameBuffer()
{
	return rtvHandle;
}


D3D12_CPU_DESCRIPTOR_HANDLE Graphics::GetDsvFrameBuffer()
{
	return dsvHandle;
}

ID3D12Resource* Graphics::GetRtvFrameResource()
{
	return rtvFrameBuffer[rtvFrameBackBufferIdx];
}

ID3D12Resource* Graphics::GetDsvFrameResource()
{
	return dsvFrameBuffer;
}

ID3D12DescriptorHeap* Graphics::CreateDescriptorHeap(UINT descNum, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	ID3D12DescriptorHeap* descHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc;
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = descNum;
	descHeapDesc.Type = type;
	auto result = _device->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(&descHeap)
	);
	ResultErrorMessage(result, L"ディスクリプタヒープの生成に失敗しました");

	return descHeap;
}


DXGI_FORMAT Graphics::GetFrameDsvFormat()
{
	return dsvFormat;
}

D3D12_VIEWPORT& Graphics::GetViewport()
{
	return viewport;
}

D3D12_RECT& Graphics::GetScissorRect()
{
	return scissor;
}

HWND Graphics::GetWindowHWND()
{
	return window.GetHWND();
}

void Graphics::BeginRender()
{
	rtvFrameBackBufferIdx = swapchain->GetCurrentBackBufferIndex();
	
	//ビューポートをセット
	cmdList->RSSetViewports(1, &viewport);
	cmdList->RSSetScissorRects(1, &scissor);

	rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += rtvDescriptorSize * rtvFrameBackBufferIdx;//バックバッファーのアドレス取得

	dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	auto resourceBarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvFrameBuffer[rtvFrameBackBufferIdx],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	cmdList->ResourceBarrier(1,&resourceBarrierDesc);

	//レンダーターゲットを設定
	cmdList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
	//レンダーターゲットをクリア
	cmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	cmdList->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr
	);
	cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Graphics::EndRender()
{
	cmdQueue->Signal(fence, ++fenceVal);
	if (fence->GetCompletedValue() != fenceVal)
	{
		auto eve = CreateEvent(nullptr, false, false, nullptr);
		fence->SetEventOnCompletion(fenceVal, eve);
		WaitForSingleObject(eve, INFINITE);
		CloseHandle(eve);
	}

	swapchain->Present(1, 0);

	ReleaseResource();
	//リセット
	cmdAlloc->Reset();
	cmdList->Reset(cmdAlloc, pipelineState);
}

void Graphics::ExecuteCommand()
{
	auto resourceBarrierDesc = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvFrameBuffer[rtvFrameBackBufferIdx],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	cmdList->ResourceBarrier(1, &resourceBarrierDesc);

	cmdList->Close();
	ID3D12CommandList* cmdlists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdlists);

}

void Graphics::SetFrameClearColor(float* clearColor)
{
	for (int i = 0; i < 4; i++)
		this->clearColor[i] = clearColor[i];
}

void Graphics::PushReleaseResource(DxObject resource)
{
	deleteList.push_back(resource);
}

void Graphics::ReleaseResource()
{
	for (auto& a : deleteList)
	{
		auto type = a.resourceType;
		switch (type)
		{
		case DxObjectType_Resource:
			a.resource->Release();
			break;
		case DxObjectType_DescriptorHeap:
			a.descHeap->Release();
			break;
		case DxObjectType_RootSignature:
			a.rootSig->Release();
			break;
		case DxObjectType_PipelineState:
			a.pipeline->Release();
			break;
		default:
			break;
		}
	}
	deleteList.clear();
}

const int Graphics::GetFrameBufferWidth()
{
	return win_width;
}

const int Graphics::GetFrameBufferHight()
{
	return win_height;
}

const int Graphics::GetBackBufferIndex()
{
	return swapchain->GetCurrentBackBufferIndex();
}

float Graphics::GetFrameBufferAspect()
{
	return (float)win_width / (float)win_height;
}



Window::~Window()
{
	UnregisterClass(wndClass.lpszClassName, wndClass.hInstance);
}

void Window::Create(int width, int height)
{
	wndClass = {};
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = (WNDPROC)WindowProcedure;
	wndClass.lpszClassName = _T("xddxdxdxd");
	wndClass.hInstance = GetModuleHandle(nullptr);
	RegisterClassEx(&wndClass);
	RECT wrc = { 0,0,width,height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	hwnd = CreateWindow(wndClass.lpszClassName,
		_T("dxdxdxdx"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wndClass.hInstance,
		nullptr
	);
	ShowWindow(hwnd, SW_SHOW);
}

HWND Window::GetHWND()
{
	return hwnd;
}

bool Window::ProcessMessage()
{
	MSG msg = {};
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT)
	{
		return false;
	}
	return true;
}


bool DispatchWindowMessage()
{
	MSG msg = {};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			break;
		}
	}
	int i = 0;
	if (msg.message== WM_KEYDOWN)
		i++;

	return msg.message != WM_QUIT;
	
}

void Graphics2::Init()
{
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.NodeMask = 0;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	EnableDebugLayer();
	window.Create(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	auto result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device));
	ResultErrorMessage(result, L"デバイスの生成に失敗しました");
	result = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	ResultErrorMessage(result, L"ファクトリの生成に失敗しました");
}

ID3D12Device5* Graphics2::GetDevice()
{
	return device;
}

IDXGIFactory4* Graphics2::GetFactory()
{
	return factory;
}

HWND Graphics2::GetWindowHWND()
{
	return window.GetHWND();
}

Graphics2::Graphics2()
{
}

Graphics2::~Graphics2()
{
	factory->Release();
	device->Release();
}


