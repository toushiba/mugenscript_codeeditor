#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <queue>
#include "node editor/imgui_node_editor.h"
#include "jp_font.h"

struct ImVec3
{
	float x;
	float y;
	float z;
};



namespace ImGuiWrapper
{
	using namespace ax::NodeEditor;
	struct ImGuiWindow;
	struct ImGuiStates;

	UINT RegistResource(ID3D12Resource* resource);
	void RegistResource(ID3D12Resource* resource, UINT idx);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUResource(UINT idx);
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUResource(UINT idx);

	

	bool InitImGui(ID3D12Device*,ID3D12GraphicsCommandList*, HWND);
	void BeginRenderImgui(const char* label);
	void EndRenderImgui(ID3D12GraphicsCommandList* commandList);
	void BeginNodeEditor();
	void EndNodeEditor();
	void BeginCreateNode();
	void EndCreateNode();
	void BeginDeleteNode();
	void EndDeleteaNode();
	void CreateNodeEditor(const char* jsonFileName);
	void SetWindowSize(float x, float y);
	void SetWindowPos(float x, float y);

	ax::NodeEditor::EditorContext* GetContext();
}

struct MyImGui
{
	bool Init(ID3D12Device*, ID3D12GraphicsCommandList*, HWND);
	void End();
	bool BeginWindow(const char* label, ImGuiWindowFlags windowFlags = 0);
	void EndWindow();
	void BeginRender();
	void EndRender();
	bool IsInit();
private:
	bool init;
	ID3D12GraphicsCommandList* commandList;
};
