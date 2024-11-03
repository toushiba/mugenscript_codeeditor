#include "MugenScriptResources.h"

MugenScript::MugenScriptResourceHandle* MugenScript::MugenScriptResourceAbstruct::SetChildResource(MugenScriptResourceID id, MugenScriptResourceHandle* handle)
{
	ChildData data;
	data.handle = handle;
	data.id = id;
	assert(!childs.find(data));
	childs.insert(data);
	return handle;
}

void MugenScript::MugenScriptResourceAbstruct::RemoveChildResource(MugenScriptResourceID id)
{
	ChildData data;
	data.id = id;
	childs.remove(data);
}

MugenScript::MugenScriptResourceHandle* MugenScript::MugenScriptResourceAbstruct::Load(int capacity, void* dst, ResourceAbstructState& outState)
{
	MugenScriptResourceHandle* ret = nullptr;
	for (int i = 0; i < childs.size(); ++i)
	{
		ret = childs[i].handle->Load(capacity - Size(), dst, outState);
		if (outState != ResourceAbstructState_Complete)
			return ret;
	}
	return ret;
}

MugenScript::MugenScriptResourceHandle* MugenScript::MugenScriptResourceAbstruct::Save(int capacity, const void* src, ResourceAbstructState& outState) const
{
	MugenScriptResourceHandle* ret = nullptr;
	for (int i = 0; i < childs.size(); ++i)
	{
		ret = childs[i].handle->Save(capacity - Size(), src, outState);
		if (outState != ResourceAbstructState_Complete)
			return ret;
	}
	return ret;
}

int MugenScript::MugenScriptResourceAbstruct::SizeTotal() const
{
	auto ret = Size();
	for (int i = 0; i < childs.size(); ++i)
		ret += childs[i].handle->SizeTotal();
	return ret;
}

void ImGuiResourceEditor::ImGuiResourceEditorApplication::Update()
{
	viewWindow.Update();
}

void ImGuiResourceEditor::ImGuiResourceEditorApplication::CreateNode()
{
	viewWindow.AddNode();
}

void ImGuiResourceEditor::ImGuiResourceEditorApplication::SetEditFile(Mugenshuen::string_t filePath)
{
	viewWindow.SetEditFile(filePath);
}

void ImGuiResourceEditor::ImGuiResourceEditorViewWindow::Draw()
{
	ImGui::Text("editor");
	ImGuiWrapper::BeginNodeEditor();
	for (int i = 0; i < nodeList.size(); ++i)
		nodeList[i]->Update(i + 1);


	auto selectedPin = NodeEditor::GetDoubleClickedPin();
	if (this->selectedPin && selectedPin)
	{
		NodeEditor::Link(++linkID, this->selectedPin, selectedPin);
		NodeEditor::EndCreate();
	}
	if (selectedPin)
	{
		this->selectedPin = selectedPin;
		if (NodeEditor::BeginCreate())
		{
		}
	}


	ImGuiWrapper::EndNodeEditor();
}

void ImGuiResourceEditor::ImGuiResourceEditorViewWindow::SetEditFile(Mugenshuen::string_t filePath)
{
	ImGuiWrapper::CreateNodeEditor(filePath.C_Str());
	this->filePath = filePath;
}

void ImGuiResourceEditor::ImGuiResourceEditorViewWindow::AddNode()
{
	nodeList.push_back(new ImGuiResourceEditorNodeTexture);
}

ImGuiWindowFlags ImGuiResourceEditor::ImGuiResourceEditorViewWindow::Flags() const
{
	return ImGuiWindowFlags();
}

const Mugenshuen::string_t ImGuiResourceEditor::ImGuiResourceEditorViewWindow::Name() const
{
	return "editor_window";
}

void ImGuiResourceEditor::ImGuiResourceEditorWindowBase::Update()
{
	if (ImGui::Begin(Name().C_Str()))
	{
		Draw();
	}
	ImGui::End();
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeTexture::Draw()
{
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeTexture::SetTexture(Mugenshuen::string_t path)
{
}

Mugenshuen::string_t ImGuiResourceEditor::ImGuiResourceEditorNodeTexture::Name()
{
	return "texture";
}

ImGuiResourceEditor::ImGuiResourceEditorNodeTexture::ImGuiResourceEditorNodeTexture()
{
	CreatePin("##", NodeEditor::PinKind::Output);
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeBase::Update(NodeEditor::NodeId id)
{
	NodeEditor::BeginNode(id);
	ImGui::Text(Name().C_Str());
	Draw();

	for (int i = 0; i < pinList.size(); ++i)
	{
		pinList[i].Begin();
		if (pinList[i].kind == NodeEditor::PinKind::Input)
			ImGui::Text("-> %s", pinList[i].name.C_Str());
		if (pinList[i].kind == NodeEditor::PinKind::Output)
			ImGui::Text("%s ->", pinList[i].name.C_Str());

		pinList[i].End();
	}
	NodeEditor::EndNode();
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeBase::CreatePin(Mugenshuen::string_t name, NodeEditor::PinKind kind)
{
	static int id = 1;
	Pin pin;
	pin.name = name;
	pin.kind = kind;
	pin.id = id++;
	pinList.push_back(pin);
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeBase::RemovePin(Mugenshuen::string_t name)
{
	for (int i = 0; i < pinList.size(); ++i)
		if (pinList[i].name == name)
		{
			pinList[i] = pinList.back();
			pinList.pop_back();
		}
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeBase::Pin::Begin()
{
	NodeEditor::BeginPin(id, kind);
	selected = ImGui::IsItemClicked();
}

void ImGuiResourceEditor::ImGuiResourceEditorNodeBase::Pin::End()
{
	NodeEditor::EndPin();
}
