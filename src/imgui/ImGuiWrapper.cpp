#include "imgui/ImGuiWrapper.h"
#include "imgui/imgui_include.h"
#include <vector>
#include<string>

using namespace ax;

struct ImGuiStates
{
	ID3D12Device* device;
	ID3D12DescriptorHeap* DescHeap;
	UINT numShaderResource;
	UINT descriptorSize;
	bool beginImGui;
	bool beginMenu;
	bool beginItem;
	bool beginPopup;
	bool beginCreateLink;
}static imguiStates;

enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

struct Node;


struct Pin
{
	NodeEditor::PinId id;
	Node* node;
	std::string name;
	NodeEditor::PinKind kind;
	PinType type;
	Pin(int id, const char* name, PinType type) :
		id(id), node(nullptr), kind(NodeEditor::PinKind::Input), type(type) {}
};

struct Link
{
	NodeEditor::LinkId id;

	NodeEditor::PinId stratPinId;
	NodeEditor::PinId endPinId;

	ImColor color;

};

struct Node
{
	NodeEditor::NodeId id;
	std::vector<Pin> inputs;
	std::vector<Pin> output;
	ImColor color;
	ImVec2 size;

	std::string state;
	std::string savedState;

	Node(int id, const char* name) :id(id), state(name) {}
};


struct ImGuiLists
{
	std::vector<Pin> pinList;
	std::vector<Link> links;
};

struct NodeEditorWrapper
{
	NodeEditor::EditorContext* context;
}static nodeEditor;


UINT ImGuiWrapper::RegistResource(ID3D12Resource* resource)
{
	auto device = imguiStates.device;
	auto descSize = imguiStates.descriptorSize;
	auto numDescriptor = imguiStates.numShaderResource;
	auto descheapCpuHandle = imguiStates.DescHeap->GetCPUDescriptorHandleForHeapStart();
	descheapCpuHandle.ptr += descSize * numDescriptor;
	D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
	view.Format = resource->GetDesc().Format;
	view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	view.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(
		resource,
		&view,
		descheapCpuHandle
	);
	imguiStates.numShaderResource++;
	return numDescriptor;
}

void ImGuiWrapper::RegistResource(ID3D12Resource* resource, UINT idx)
{
	auto device = imguiStates.device;
	auto descSize = imguiStates.descriptorSize;
	auto descHeapCpuHandle = imguiStates.DescHeap->GetCPUDescriptorHandleForHeapStart();
	descHeapCpuHandle.ptr += descSize * idx;

	D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
	view.Format = resource->GetDesc().Format;
	view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	view.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(
		resource,
		&view,
		descHeapCpuHandle
	);
}

D3D12_CPU_DESCRIPTOR_HANDLE ImGuiWrapper::GetCPUResource(UINT idx)
{
	assert(idx < imguiStates.numShaderResource);

	auto start = imguiStates.DescHeap->GetCPUDescriptorHandleForHeapStart();
	start.ptr+=idx * imguiStates.descriptorSize;
	return start;
}

D3D12_GPU_DESCRIPTOR_HANDLE ImGuiWrapper::GetGPUResource(UINT idx)
{
	assert(idx < imguiStates.numShaderResource);

	auto start = imguiStates.DescHeap->GetGPUDescriptorHandleForHeapStart();
	start.ptr += idx * imguiStates.descriptorSize;
	return start;
}

bool ImGuiWrapper::InitImGui(ID3D12Device* _device,ID3D12GraphicsCommandList* cmdList, HWND window)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiStates.DescHeap));
	if (FAILED(result))
	{
		MessageBox(nullptr, L"imgui‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½", L"error", MB_OK);
		return false;
	}
	auto context = ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	bool initresult = ImGui_ImplWin32_Init(window);
	initresult = ImGui_ImplDX12_Init(
		_device,
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		imguiStates.DescHeap,
		imguiStates.DescHeap->GetCPUDescriptorHandleForHeapStart(),
		imguiStates.DescHeap->GetGPUDescriptorHandleForHeapStart()
	);
	if (!initresult)
	{
		MessageBox(nullptr, L"imgui‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½", L"error", MB_OK);
		return false;
	}

	return true;
}

void ImGuiWrapper::BeginRenderImgui(const char* label)
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	imguiStates.beginImGui = ImGui::Begin(label);
}

void ImGuiWrapper::EndRenderImgui(ID3D12GraphicsCommandList* commandList)
{
	static auto cmdList = commandList;
	static auto descHeap = imguiStates.DescHeap;
	if (imguiStates.beginImGui)
		ImGui::End();
	ImGui::Render();
	cmdList->SetDescriptorHeaps(1, &descHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void ImGuiWrapper::BeginNodeEditor()
{
	ImGui::Separator();
	SetCurrentEditor(ImGuiWrapper::GetContext());
	Begin("Editor");
}

void ImGuiWrapper::EndNodeEditor()
{
	End();
	SetCurrentEditor(nullptr);
}

void ImGuiWrapper::BeginCreateNode()
{
	imguiStates.beginCreateLink = BeginCreate();
}

void ImGuiWrapper::EndCreateNode()
{
	if (imguiStates.beginCreateLink)
		ax::NodeEditor::EndCreate();
}

void ImGuiWrapper::BeginDeleteNode()
{
	ax::NodeEditor::BeginDelete();
}

void ImGuiWrapper::EndDeleteaNode()
{
	ax::NodeEditor::EndDelete();
}

ax::NodeEditor::EditorContext* ImGuiWrapper::GetContext()
{
	return nodeEditor.context;
}


void ImGuiWrapper::CreateNodeEditor(const char* jsonFileName)
{
	ax::NodeEditor::Config config;
	config.SettingsFile = jsonFileName;
	nodeEditor.context = ax::NodeEditor::CreateEditor(&config);

}

void ImGuiWrapper::SetWindowSize(float x, float y)
{
	ImGui::SetWindowSize({ x,y }, ImGuiCond_::ImGuiCond_FirstUseEver);
}

void ImGuiWrapper::SetWindowPos(float x, float y)
{
	ImGui::SetWindowPos({ x,y }, ImGuiCond_::ImGuiCond_FirstUseEver);
}



bool MyImGui::Init(ID3D12Device* _device, ID3D12GraphicsCommandList* commandList, HWND window)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 16;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	auto result = _device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&imguiStates.DescHeap));
	if (FAILED(result))
	{
		MessageBox(nullptr, L"imgui‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½", L"error", MB_OK);
		return false;
	}
	auto context = ImGui::CreateContext();
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = false;
	bool initresult = ImGui_ImplWin32_Init(window);
	initresult = ImGui_ImplDX12_Init(
		_device,
		3,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		imguiStates.DescHeap,
		imguiStates.DescHeap->GetCPUDescriptorHandleForHeapStart(),
		imguiStates.DescHeap->GetGPUDescriptorHandleForHeapStart()
	);
	if (!initresult)
	{
		MessageBox(nullptr, L"imgui‚Ì‰Šú‰»‚ÉŽ¸”s‚µ‚Ü‚µ‚½", L"error", MB_OK);
		return false;
	}

	this->commandList = commandList;
	imguiStates.device = _device;
	imguiStates.descriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	imguiStates.numShaderResource = 1;
	init = true;

	io.Fonts->AddFontFromFileTTF("resource/meiryo.ttc", 18.0f, nullptr, glyphRangesJapanese);

	return true;
}

void MyImGui::End()
{

}

bool MyImGui::BeginWindow(const char* label, ImGuiWindowFlags windowFlags)
{
	return ImGui::Begin(label, 0, windowFlags);
}

void MyImGui::EndWindow()
{
	ImGui::End();
}

void MyImGui::BeginRender()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void MyImGui::EndRender()
{
	static auto descHeap = imguiStates.DescHeap;

	ImGui::Render();
	commandList->SetDescriptorHeaps(1, &descHeap);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

bool MyImGui::IsInit()
{
	return init;
}

