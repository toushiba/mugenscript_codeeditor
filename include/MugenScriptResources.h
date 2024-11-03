#pragma once
#include "Mugenshuen.h"
#include "imgui/ImGuiWrapper.h"
namespace MugenScript
{
	using MugenScriptResourceID = int;
	class MugenScriptResourceHandle;
	enum RESOURCE_TYPE
	{
		RESOURCE_TYPE_TEXTURE,
		RESOURCE_TYPE_BGM,
		RESOURCE_TYPE_SE
	};

	enum ResourceAbstructState
	{
		ResourceAbstructState_None,
		ResourceAbstructState_Complete,
		ResourceAbstructState_Pending,
		ResourceAbstructState_Fail,
	};

	struct MugenScriptResourceHandle
	{
		virtual MugenScriptResourceHandle* SetChildResource(MugenScriptResourceID id,MugenScriptResourceHandle* handle) = 0;
		virtual void RemoveChildResource(MugenScriptResourceID id) = 0;
		virtual void Preview() = 0;
		virtual MugenScriptResourceHandle* Load(int capacity,void*dst,ResourceAbstructState& outState) = 0;
		virtual MugenScriptResourceHandle* Save(int capacity,const void*src, ResourceAbstructState& outSstate)const = 0;
		virtual int Size()const = 0;
		virtual int SizeTotal()const = 0;
	};

	class MugenScriptResourceAbstruct:public MugenScriptResourceHandle
	{
	public:
		struct ChildData
		{
			MugenScriptResourceID id;
			MugenScriptResourceHandle* handle;
			bool operator==(const ChildData& o)const { return id == o.id; }
			bool operator<=(const ChildData& o)const { return id <= o.id; }
			bool operator>=(const ChildData& o)const { return id >= o.id; }
			bool operator<(const ChildData& o)const { return id < o.id; }
			bool operator>(const ChildData& o)const { return id > o.id; }
			void operator=(const ChildData& o) { handle = o.handle; }
		};
		MugenScriptResourceHandle* SetChildResource(MugenScriptResourceID id, MugenScriptResourceHandle* handle)override final;
		void RemoveChildResource(MugenScriptResourceID id) override final;
		MugenScriptResourceHandle* Load(int capacity, void* dst, ResourceAbstructState& outState) override;
		MugenScriptResourceHandle* Save(int capacity, const void* src, ResourceAbstructState& outState)const override;
		int SizeTotal()const override final;

	private:
		MugenScriptResourceHandle* parent;
		Mugenshuen::btree_t<ChildData> childs;
	};


	class MugenScriptResourceHeader
	{
	public:

		struct TextureTransform
		{
			float offsetX;
			float offsetY;
			float offsetZ;
			float rotationX;
			float rotationY;
			float rotationZ;
			float scaleX;
			float scaleY;
			float scaleZ;
		};

		struct SoundProperty
		{
			char volume;
			char samplePoint;
			char loop;
			char speed;
		};

		struct ScriptCallname
		{
			Mugenshuen::string_t callName;
		};

		union HeaderData
		{
			TextureTransform textureTransform;
			SoundProperty sound;
		};

		Mugenshuen::string_t name;
		RESOURCE_TYPE type;
		HeaderData header;

		void operator=(const MugenScriptResourceHeader&) = delete;
		void operator=(MugenScriptResourceHeader&&) = delete;
		MugenScriptResourceHeader(RESOURCE_TYPE type);
		MugenScriptResourceHeader(const MugenScriptResourceHeader&) = delete;
		MugenScriptResourceHeader(MugenScriptResourceHeader&&) = delete;

	};

	class MugenScriptResourceBody
	{
	public:

		int size;
		char* data;

		void operator=(const MugenScriptResourceBody&) = delete;
		void operator=(MugenScriptResourceBody&&) = delete;
		MugenScriptResourceBody();
		MugenScriptResourceBody(const MugenScriptResourceBody&) = delete;
		MugenScriptResourceBody(MugenScriptResourceBody&&) = delete;
	};

	class MugenScriptResourceObject :public MugenScriptResourceAbstruct
	{
	public:

		MugenScriptResourceHandle* Load(int capacity, void* dst, ResourceAbstructState& outState) override;
		MugenScriptResourceHandle* Save(int capacity, const void* src, ResourceAbstructState& outState)const override;
		void Preview()override;
		int Size()const override;
		MugenScriptResourceObject(MugenScriptResourceID id, RESOURCE_TYPE type);
	private:

		MugenScriptResourceID id;
	};

}

namespace ImGuiResourceEditor
{
	using namespace ax;

	class ImGuiResourceEditorWindowBase
	{
	public:
		void Update();
		virtual void Draw() = 0;
		virtual ImGuiWindowFlags Flags()const = 0;
		virtual const Mugenshuen::string_t Name()const = 0;
	};

	class ImGuiResourceEditorNodeBase
	{
	public:


		struct Pin
		{
			NodeEditor::PinId id;
			Mugenshuen::string_t name;
			NodeEditor::PinKind kind;
			bool selected;
			void Begin();
			void End();
		};

		void Update(NodeEditor::NodeId id);
		virtual void Draw() = 0;
		virtual void DrawPin(Mugenshuen::string_t) {};
		virtual Mugenshuen::string_t Name() = 0;

		void CreatePin(Mugenshuen::string_t name, NodeEditor::PinKind kind);
		void RemovePin(Mugenshuen::string_t name);
	private:
		Mugenshuen::vector_t<Pin> pinList;
	};

	class ImGuiResourceEditorNodeTexture :public ImGuiResourceEditorNodeBase
	{
	public:
		void Draw()override;
		void SetTexture(Mugenshuen::string_t path);
		Mugenshuen::string_t Name();
		ImGuiResourceEditorNodeTexture();
	private:

	};

	class ImGuiResourceEditorViewWindow:public ImGuiResourceEditorWindowBase
	{
	public:

		struct Link
		{
			NodeEditor::PinId source;
			NodeEditor::PinId destination;
		};
		void Draw()override;
		void SetEditFile(Mugenshuen::string_t filePath);
		void AddNode();
		ImGuiWindowFlags Flags()const override;
		const Mugenshuen::string_t Name()const override;
	private:
		Mugenshuen::string_t filePath;
		Mugenshuen::vector_t<ImGuiResourceEditorNodeBase*> nodeList;
		int linkID;
		NodeEditor::PinId selectedPin;
	};

	class ImGuiResourceEditorApplication
	{
	public:

		void Update();
		void CreateNode();
		void SetEditFile(Mugenshuen::string_t filePath);

	private:
		ImGuiResourceEditorViewWindow viewWindow;

	};
}