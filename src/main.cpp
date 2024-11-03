#include <time.h>
#include <iostream>
#include "common/FileSystem.h"
#include "imgui/ImGuiWrapper.h"
#include "ResDx.h"
#include "DirectX/Graphics.h"
#include "MugenScriptDatabase.h" 
#include "Imgui_Database.h"
#include "DxLibDebug.h"
#include "MugenScriptResources.h"

#pragma comment(lib, "winmm.lib")

using namespace Microsoft;
using namespace WRL;
using namespace ResDx;

int main()  
{

	ResDxInit();
	ImGuiDatabase::ImGuiDatabaseAppication dbApp;
	ResDxCore::ResDxCommandListDevice commandList;
	ResDxCore::ResDxRendererDevice renderer(commandList);
	ResDxContext2 context(&graphics2);
	auto list = commandList.CreateCommandList(graphics2.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto& tableDescriptorHeap = ResDxDescriptorHeapTable::instance();
	auto& rootSig = ResDxRendererRootSignature::instance();
	MyImGui imgui;
	imgui.Init(graphics2.GetDevice(), list->Get(), graphics2.GetWindowHWND());
	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigInputTextEnterKeepActive = true;
	ResDxCommandListDirect directCommandList(list);

	while (DispatchWindowMessage())
	{
		renderer.StartFrame();
		renderer.SetFrameRenderTargetView(list);
		imgui.BeginRender();

		dbApp.Update();
		
		imgui.EndRender();
		list->Close();
		renderer.PushCommandList(list);
		renderer.EndFrame();
		
		commandList.Reset(list);
		commandList.ResetAllocator();
	}
	//Job::EndJobSystem();
	return 0;
}

