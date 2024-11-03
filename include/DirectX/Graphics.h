#pragma once
#include <Windows.h>
#include <crtdbg.h>
#include <tchar.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXTex.h>
#include <vector>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"d3dcompiler.lib")
#include "Buffer/Allocator.h"
#define FRAME_BUFFER_BYTESIZE 2048
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define BUFFER_COUNT 2

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		return true;

	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void ResultErrorMessage(HRESULT result, const wchar_t* message)
{
	if (FAILED(result))
	{
		MessageBox(nullptr, message, L"error", MB_OK);
		std::abort();
	}
}

static void EnableDebugLayer()
{
	ID3D12Debug* debug = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(&debug)
	);
	debug->EnableDebugLayer();
	debug->Release();
}

inline UINT Alignmentof(UINT byteSize)
{
	return (byteSize + 256) & 0xFFFFFF00;
}

bool DispatchWindowMessage();


enum DxObjectType
{
	DxObjectType_Resource=0,
	DxObjectType_DescriptorHeap,
	DxObjectType_RootSignature,
	DxObjectType_PipelineState,
};

struct DxObject
{
	union
	{
		ID3D12Resource* resource;
		ID3D12DescriptorHeap* descHeap;
		ID3D12PipelineState* pipeline;
		ID3D12RootSignature* rootSig;
	};

	DxObjectType resourceType;

	DxObject(DxObjectType type, ID3D12Resource* res) :resourceType(type), resource(res) {};
	DxObject(DxObjectType type, ID3D12DescriptorHeap* res) :resourceType(type), descHeap(res) {};
	DxObject(DxObjectType type, ID3D12PipelineState* res) :resourceType(type), pipeline(res) {};
	DxObject(DxObjectType type, ID3D12RootSignature* res) :resourceType(type), rootSig(res) {};
};

class Window
{
	HWND hwnd;
	WNDCLASSEX wndClass;
public:
	~Window();
	void Create(int width, int height);
	HWND GetHWND();
	bool ProcessMessage();
};

class Graphics
{
	const int win_width = 1280;
	const int win_height = 720;
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;
	UINT rtvFrameBackBufferIdx;
	UINT fenceVal = 0;
	Window window;
	ID3D12Device5* _device = nullptr;
	ID3D12CommandAllocator* cmdAlloc = nullptr;
	ID3D12GraphicsCommandList4* cmdList = nullptr;
	ID3D12CommandQueue* cmdQueue = nullptr;
	ID3D12Resource* rtvFrameBuffer[2];
	ID3D12Resource* dsvFrameBuffer = nullptr;
	ID3D12DescriptorHeap* rtvHeap = nullptr;
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	ID3D12DescriptorHeap* heapForImgui = nullptr;
	ID3D12PipelineState* pipelineState = nullptr;
	IDXGIFactory4* factory = nullptr;
	IDXGISwapChain3* swapchain = nullptr;
	ID3D12Fence* fence = nullptr;
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissor = {};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	DXGI_FORMAT dsvFormat= DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	std::vector<DxObject> deleteList;

	float clearColor[4] = { 0.0f,0.0f,0.0f,0.0f };

	void ActiveDebugLayer();
	HRESULT CreateCmdQueue();
	HRESULT CreateSwapChain();
	HRESULT CreateDescriptorHeapFrameBuffer();

public:

	~Graphics();
	void Init();
	ID3D12Device5* GetDevice();
	ID3D12CommandAllocator* GetCmdAlloc();
	ID3D12GraphicsCommandList4* GetCmdList();
	ID3D12CommandQueue* GetCmdQueue();
	IDXGIFactory4* GetFactory();
	IDXGISwapChain3* GetSwapchain();
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvFrameBuffer();
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvFrameBuffer();
	ID3D12Resource* GetRtvFrameResource();
	ID3D12Resource* GetDsvFrameResource();
	ID3D12DescriptorHeap* CreateDescriptorHeap(UINT descNum, D3D12_DESCRIPTOR_HEAP_TYPE type);
	DXGI_FORMAT GetFrameDsvFormat();
	D3D12_VIEWPORT& GetViewport();
	D3D12_RECT& GetScissorRect();
	HWND GetWindowHWND();
	void BeginRender();
	void EndRender();
	void ExecuteCommand();
	void SetFrameClearColor(float* clearColor);
	void PushReleaseResource(DxObject);
	void ReleaseResource();
	const int GetFrameBufferWidth();
	const int GetFrameBufferHight();
	const int GetBackBufferIndex();
	float GetFrameBufferAspect();
};

struct GraphicsDevice
{
	virtual ID3D12Device5* GetDevice() = 0;
	virtual IDXGIFactory4* GetFactory() = 0;
	virtual HWND GetWindowHWND() = 0;
};

class Graphics2:public GraphicsDevice
{
public:

	void Init();
	ID3D12Device5* GetDevice()override;
	IDXGIFactory4* GetFactory()override;
	HWND GetWindowHWND()override;

	Graphics2();
	~Graphics2();
private:

	void operator=(const Graphics2&) = delete;
	void operator=(Graphics2&&) = delete;

	Graphics2(const Graphics2&) = delete;
	Graphics2(Graphics2&&) = delete;

	ID3D12Device5* device;
	IDXGIFactory4* factory;
	Window window;
};


class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class ShaderResourceBuffer;
class DescirptorHeap;
class RenderTarget;
class DepthStencil;
class TextureResource;
class DescriptorHeap;

extern Graphics graphics;
extern Graphics2 graphics2;