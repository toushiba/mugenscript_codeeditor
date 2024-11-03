#pragma once
#include "imgui/ImGuiWrapper.h"
#include "MugenScriptDatabase.h"
#include "DxLibDebug.h"

namespace ImGuiDatabase
{

}

namespace ImGuiDatabase
{
	using DatabaseWindowFlags = unsigned int;
	class ImGuiDatabaseAppication;

	enum DATABASAE_WINDOW_COMMAND
	{
		DATABASAE_WINDOW_COMMAND_NONE,
		DATABASAE_WINDOW_COMMAND_DO,
		DATABASAE_WINDOW_COMMAND_REDO,
		DATABASAE_WINDOW_COMMAND_UNDO,
		DATABASAE_WINDOW_COMMAND_CLEAR_QUEUE,
	};

	enum DatabaseWindowFlags_
	{
		DatabaseWindowFlags_None,
		DatabaseWindowFlags_UpdateFocused,
		DatabaseWindowFlags_InputFocused,
	};

	enum ImGuiDatabaseApplitionEvent
	{
		ImGuiDatabaseEvent_OpenDatabaseWindow,
		ImGuiDatabaseEvent_OpenTableWindow,
		ImGuiDatabaseEvent_OpenRecordWindow,
		ImGuiDatabaseEvent_OpenRelationalWindow,
		ImGuiDatabaseEvent_OpenConditionWindow,
		ImGuiDatabaseEvent_OpenCreateDatabaseWindow,
		ImGuiDatabaseEvent_OpenCreateTableWindow,
		ImGuiDatabaseEvent_OpenCreateRelationalWindow,
		ImGuiDatabaseEvent_OpenCreateConditionWindow,
		ImGuiDatabaseEvent_OpenDebugWindow,
		ImGuiDatabaseEvent_OpenDatabase,
		ImGuiDatabaseEvent_OpenTable,
		ImGuiDatabaseEvent_CloseDatabaseWindow,
		ImGuiDatabaseEvent_CloseTableWindow,
		ImGuiDatabaseEvent_CloseRecordWindow,
		ImGuiDatabaseEvent_CloseRelationalWindow,
		ImGuiDatabaseEvent_CloseConditionWindow,
		ImGuiDatabaseEvent_CloseCreateDatabaseWindow,
		ImGuiDatabaseEvent_CloseCreateTableWindow,
		ImGuiDatabaseEvent_CloseCreateConditionWindow,
		ImGuiDatabaseEvent_CloseCreateRelationalWindow,
		ImGuiDatabaseEvent_CloseDebugWindow,
		ImGuiDatabaseEvent_CloseDatabase,
		ImGuiDatabaseEvent_CloseTable,
		ImGuiDatabaseEvent_CreateDatabase,
		ImGuiDatabaseEvent_CreateTable,
		ImGuiDatabaseEvent_CreateRecord,
		ImGuiDatabaseEvent_CreateIndex,
		ImGuiDatabaseEvent_DeleteDatabase,
		ImGuiDatabaseEvent_DeleteTable,
		ImGuiDatabaseEvent_DeleteRecord,
		ImGuiDatabaseEvent_DeleteIndex,
		ImGuiDatabaseEvent_LoadDatabase,
		ImGuiDatabaseEvent_LoadTable,
		ImGuiDatabaseEvent_SaveDatabase,
		ImGuiDatabaseEvent_SaveTable,
		ImGuiDatabaseEvent_EditRecord,
		ImGuiDatabaseEvent_Record,
		ImGuiDatabaseEvent_FilteringIndex,
		ImGuiDatabaseEvent_ClearFilteringIndex,
		ImGuiDatabaseEvent_Export,
		ImGuiDatabaseEvent_Inport,
		ImGuiDatabaseEvent_FocusWindow,
		ImGuiDatabaseEvent_InputOnryWindow
	};

	struct ImGuiDatabaseCommand
	{
		virtual void Do() = 0;
		virtual void Redo() {};
		virtual void Undo() {};
		virtual ~ImGuiDatabaseCommand() {}
	};

	struct ImGuiDatabaseCommandDummy :public ImGuiDatabaseCommand
	{
		void Do()override {};
		void Redo()override {};
		void Undo()override {};
		~ImGuiDatabaseCommandDummy() {}
	};

	struct ImGuiDatabaseCommandOpenDatabaseWindow :public ImGuiDatabaseCommand
	{
		void Do()override {};
		ImGuiDatabaseCommandOpenDatabaseWindow(ImGuiDatabaseAppication* a) :app(a) {};
		ImGuiDatabaseAppication* app;
	};


	class ImGuiDatabaseCommandLoadTable :public ImGuiDatabaseCommand
	{
	public:
		void Do()override;
		ImGuiDatabaseAppication* app;
	};

	class ImGuiDatabaseCommandCreateTable :public ImGuiDatabaseCommand
	{
	public:
		void Do()override {};
	private:
	};

	class ImGuiDatabaseWindowBase
	{
	public:
		void Update(ImGuiDatabaseAppication*, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
		virtual Mugenshuen::string_t Name() = 0;
		virtual ImGuiWindowFlags Flags() { return 0; }
	private:
		virtual void Draw(ImGuiDatabaseAppication*) {};
		virtual void DrawFocuse(ImGuiDatabaseAppication*) {};
		DatabaseWindowFlags flags;
	};

	struct MugenScriptDatabaseTableData
	{
		int numColumn;
		MugenScript::DATABASE_VALUE_TYPE* types;
		Mugenshuen::string_t* names;
		MugenScript::MugenScriptDatabaseValue* initials;
	};

}

namespace ImGuiDatabase
{
	class ImGuiCodeEditorViewer;

	enum RECORD_WINDOW_STATE
	{
		RECORD_WINDOW_STATE_NONE,
		RECORD_WINDOW_STATE_CREATE,
		RECORD_WINDOW_STATE_CANCEL,
		RECORD_WINDOW_STATE_REPLACE
	};


	class ImGuiValueInput
	{
	public:

		int Input(ImGuiDatabaseAppication* app,int record, MugenScript::DATABASE_VALUE_TYPE type,MugenScript::MugenScriptDatabaseValue& value, Mugenshuen::string_t name = "##v");
		int strCapacity;
		ImGuiValueInput() :strCapacity(),triggerSelected(),scriptEdit() {};
		~ImGuiValueInput() {}
	private:
		int triggerSelected;
		bool scriptEdit;
	};


	class ImGuiDatabaseTable
	{
	public:
		typedef int(*Callback)(void*);

		void Set(int column, const MugenScript::MugenScriptDatabaseValue* name, Callback idCallback = 0);
		void Begin(const char* label, ImGuiTableFlags);
		bool BeginColumn();
		bool BeginRecord(MugenScript::MugenScriptDatabaseValue*);
		void EndColumn();
		void EndRecord();
		void End();
		 
	private:
		Mugenshuen::string_t GetStringValue(const MugenScript::MugenScriptDatabaseValue& v);
		Mugenshuen::string_t GetStringOneLine(Mugenshuen::string_t);

		int numColumn;
		Mugenshuen::array_t<Mugenshuen::string_t> names;
		Callback callback;
		bool result;
		bool tree;
		int record;
		MugenScript::MugenScriptDatabaseValue* values;
	};

	class ImGuiRecordWindow:public ImGuiDatabaseWindowBase
	{
	public:

		void Create(ImGuiDatabaseAppication* app,int numColumn, const MugenScript::MugenScriptDatabaseValue* names, const MugenScript::DATABASE_VALUE_TYPE* type, const MugenScript::MugenScriptDatabaseValue* initials, MugenScript::MugenScriptDatabaseValue* dst);
		void Edit(ImGuiDatabaseAppication* app, int id, int numColumn, const MugenScript::MugenScriptDatabaseValue* names, const MugenScript::DATABASE_VALUE_TYPE* type, MugenScript::MugenScriptDatabaseValue* dst);
		void Close();
		int EditID();
		bool IsOpen()const;
		RECORD_WINDOW_STATE State()const;
		void Draw(ImGuiDatabaseAppication*)override;
		void DrawFocuse(ImGuiDatabaseAppication*)override {};
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
	private:
		RECORD_WINDOW_STATE state;
		bool first;
		int id;
		bool open;
		int numColumn;
		const MugenScript::MugenScriptDatabaseValue* names;
		const MugenScript::DATABASE_VALUE_TYPE* types;
		MugenScript::MugenScriptDatabaseValue* record;
		ImGuiValueInput* input;
		Mugenshuen::string_t buttonName;
		RECORD_WINDOW_STATE resultState;
	};


	class ImGuiDatabaseTableView:public ImGuiDatabaseWindowBase
	{
	public:
		Mugenshuen::string_t Name()override;
		void Draw(ImGuiDatabaseAppication*)override;
		void DrawFocuse(ImGuiDatabaseAppication*)override;

	private:
		int select;
		int selectColumn;
		int recordForcus;
		
		ImGuiDatabaseTable imguiTable;
	};

	class ImGuiDatabaseDirectoryWindow
	{
	public:
		void Update(MugenScript::MugenScriptDatabaseEditor* editor,int(callback)(MugenScript::MugenScriptDatabaseEditor* ,Mugenshuen::string_t&));

		using file_it = FileSystem::FileManager::FileIT;
		ImGuiDatabaseDirectoryWindow();
	private:

		file_it it;
	};

	class ImGuiDatabaseWindow:public ImGuiDatabaseWindowBase
	{
	public:
		void Clear();
		Mugenshuen::string_t Name()override;
		void Draw(ImGuiDatabaseAppication*)override;
		void DrawFocuse(ImGuiDatabaseAppication*)override;
		ImGuiDatabaseWindow():loadDatabase(),loadTable(),select(){}
	private:
		bool loadDatabase;
		bool loadTable;
		int select;
		int selectDatabase;
	};

	class ImGuiCreateDatabaseWindow :public ImGuiDatabaseWindowBase
	{
	public:

		void Draw(ImGuiDatabaseAppication*)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;

	private:
		char buff[32];
	};

	class ImGuiCreateRelationalWindow :public ImGuiDatabaseWindowBase
	{
	public:
		enum RELATIONAL_WINDOW_MODE
		{
			RELATIONAL_WINDOW_MODE_CREATE,
			RELATIONAL_WINDOW_MODE_EDIT,
		};
		void Create();
		void Edit();
		void Draw(ImGuiDatabaseAppication*)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
		void SetColumnName(Mugenshuen::string_t columnName);

	private:
		RELATIONAL_WINDOW_MODE state;
		MugenScript::MugenScriptDatabaseValue value[3];
		int comboSelect[3];
		ImGuiValueInput input[3]; 
		Mugenshuen::string_t columnName;
	};

	class ImGuiCreateConditionWindow :public ImGuiDatabaseWindowBase
	{
	public:

		void Draw(ImGuiDatabaseAppication* app)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
		void SetColumnID(int columnID);

	private:
		int columnID;
		int relationID;
		int comboSelect[3];
		ImGuiValueInput input[3];
		Mugenshuen::string_t columnName;
	};

	class ImGuiRelationalWindow :public ImGuiDatabaseWindowBase
	{
	public:

		void Draw(ImGuiDatabaseAppication*)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
		void SetColumnID(int columnID);
		void SetRelationalPath(Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::string_t column);
	private:
		int columnID;
		ImGuiDatabaseTable table;
		Mugenshuen::string_t path;
	};

	class ImGuiConditionWindow :public ImGuiDatabaseWindowBase
	{
	public:
		void Draw(ImGuiDatabaseAppication*)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
		void SetColumnID(int id);
	private:
		int columnID;
		ImGuiDatabaseTable table;
	};

	class ImGuiCreateTableWindow :public ImGuiDatabaseWindowBase
	{
	public:

		void Draw(ImGuiDatabaseAppication* app)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;

	private:
		void Close();
		bool CheckNamesValid();
		int numColumn;
		int selectColumn;
		MugenScript::MugenScriptDatabaseValue name;
		ImGuiValueInput nameInput;
		Mugenshuen::vector_t<Mugenshuen::string_t> names;
		Mugenshuen::vector_t<MugenScript::DATABASE_VALUE_TYPE> types;
		Mugenshuen::vector_t<MugenScript::MugenScriptDatabaseValue> initials;
		Mugenshuen::vector_t<const char*> comboString;
		Mugenshuen::vector_t<int> comboSelect;
		Mugenshuen::vector_t<ImGuiValueInput> initialInput;
	};

	class ImGuiDebugWindow :public ImGuiDatabaseWindowBase
	{
	public:

		void Draw(ImGuiDatabaseAppication* app)override;
		Mugenshuen::string_t Name()override;
		ImGuiWindowFlags Flags()override;
		void PushProcessRequest(const Mugenshuen::string_t path);
		void PushLoadScript(const Mugenshuen::string_t script);
	private:
		void StartDebugger(const Mugenshuen::string_t processPath,const Mugenshuen::string_t script);
		MugenScriptDebug::MugenScriptProcessDebuggerMaster debugger;
		Mugenshuen::queue_t<Mugenshuen::string_t> processQueue;
		Mugenshuen::queue_t<Mugenshuen::string_t> scriptQueue;
		MugenScriptDxLib::MugenScriptDxLibMachineDebugger dxLibDebugger;
		FileSystem::FileManager::FileIO file;
	};

	class ImGuiScriptManager
	{
	public:

		void Update();
		void BeginEditScript(const Mugenshuen::string_t name,const Mugenshuen::string_t script,MugenScript::MugenScriptDatabaseValue* dst);
		void EndEditScript();

	private:
		HANDLE file;
		FILETIME lastWriteTime;
		int id;
		int column;
		MugenScript::MugenScriptDatabaseTableScript script;
		Mugenshuen::string_t editScriptPath;
		MugenScript::MugenScriptDatabaseValue* value;
	};

	class ImGuiDatabaseAppication
	{
	public:

		using command_t = ImGuiDatabaseApplitionEvent;

		void Update();
		
		void OpenDatabaseWindow();
		void OpenTableWindow();
		void OpenRecordWindow();
		void OpenCreateDatabaseWindow();
		void OpenCreateTableWindow();
		void OpenCreateRelationalWindow(Mugenshuen::string_t columnName);
		void OpenCreateConditionWindow(int columnID);
		void OpenRelationalWindow(int columnID);
		void OpenConditionWindow(int columnID);
		void OpenDebugWindow(Mugenshuen::string_t processPath,Mugenshuen::string_t script);
		
		void CloseDatabaseWindow();
		void CloseTableWindow();
		void CloseRecordWindow();
		void CloseCreateDatabaseWindow();
		void CloseCreateTableWindow();
		void CloseCreateRelationalWindow();
		void CloseCreateConditionWindow();
		void CloseRelationalWindow();
		void CloseConditionWindow();
		void CloseDebugWindow();

		void LoadDatabase(Mugenshuen::string_t dbName);
		void LoadTable(Mugenshuen::string_t tblName);
		
		void SaveDatabase();
		void SaveTable();
		
		void CloseDatabase(bool save);
		void CloseTable(bool save);

		void CreateDatabase(Mugenshuen::string_t dbName);
		void CreateTable(Mugenshuen::string_t tblName, int numColumn, Mugenshuen::string_t* names, MugenScript::DATABASE_VALUE_TYPE* types, MugenScript::MugenScriptDatabaseValue* initials);
		void CreateRecord();
		void CreateRecordData(int num,MugenScript::MugenScriptDatabaseValue*);
		void CreateRelational();
		void CreateRelationalData(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, const Mugenshuen::string_t tableName, const Mugenshuen::string_t relationalColumnName);

		void DeleteDatabase(Mugenshuen::string_t dbName);
		void DeleteTable(Mugenshuen::string_t tblName);
		void DeleteRecord(int id);
		void DeleteRelational(const Mugenshuen::string_t columnName);

		void EditRecord(int id);
		void EditRecordData(int id, MugenScript::MugenScriptDatabaseValue*);
		void EditRelational();
		void EditRelationalData(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, const Mugenshuen::string_t tableName, const Mugenshuen::string_t relationalColumnName);
		void EditScript(const Mugenshuen::string_t name, const Mugenshuen::string_t& script, MugenScript::MugenScriptDatabaseValue* dst);

		void FocuseDatabaseWindow();
		void FocuseTableWindow();
		void FocuseRecordWindow();

		int GetNumDatabsaes();
		int GetNumTables();
		int GetNumRecords();
		int GetNumColumns();
		int GetNumRecordsRelation(int columnID);
		int GetRecordID(int idx);
		int GetRecordIDRelation(int idx, int columnID);
		int GetNumTables(Mugenshuen::string_t databaseName);
		bool IsLoadedDatabase(Mugenshuen::string_t dbName);

		
		void OnlyInput(Mugenshuen::string_t windowName);
		void ClearOnlyInput();
		bool IsTableExist(Mugenshuen::string_t tableName);
		
		Mugenshuen::string_t CurrentDatabase();
		Mugenshuen::string_t GetNamesDatabase(int idx);
		Mugenshuen::string_t GetNamesTable(int idx);
		MugenScript::MugenScriptDatabaseValue* GetRecord(int id);
		const MugenScript::MugenScriptDatabaseValue* GetNamesColumn();
		const MugenScript::MugenScriptDatabaseValue* GetInitials();
		const MugenScript::DATABASE_VALUE_TYPE* GetTypesColumn();
		const MugenScript::MugenScriptDatabaseValue GetInitial(int column);
		const MugenScript::MugenScriptDatabaseValue GetInitialCondition(int column);
		
		int GetNumColumnRelation(int columnID);
		void GetNamesTable(Mugenshuen::string_t database,Mugenshuen::string_t* dst);
		int GetNamesColumn(MugenScript::DATABASE_VALUE_TYPE type,Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::vector_t<Mugenshuen::string_t>& dst);
		int GetNamesColumn(Mugenshuen::string_t database, Mugenshuen::string_t table, Mugenshuen::vector_t<Mugenshuen::string_t>& dst);
		MugenScript::DATABASE_VALUE_TYPE GetTypeRelation(int columnID);
		MugenScript::DATABASE_VALUE_TYPE GetTypeRelation(Mugenshuen::string_t columnID);
		MugenScript::MugenScriptDatabaseValue GetInitialRelatioin(int columnID);
		const MugenScript::MugenScriptDatabaseValue* GetColumnNameRelation(int columnID);
		MugenScript::MugenScriptDatabaseValue* GetRecordRelational(int id,int columnID);
		void GetRelationalPath(const int columnID, Mugenshuen::string_t& database, Mugenshuen::string_t& table, Mugenshuen::string_t& column);

		bool CheckCondition(int columnID,int recordID);

	private:
		void ExecuteEvent();
		ImGuiWindowFlags GetFlag(Mugenshuen::string_t windowName)const;
		Mugenshuen::queue_t<ImGuiDatabaseApplitionEvent> eventQueue;
		Mugenshuen::queue_t<MugenScript::MugenScriptDatabaseValue> onryInput;
		bool activeDatabaseWindow;
		bool activeTableWindow;
		bool activeRecordWindow;
		bool activeRelationWindow;
		bool activeConditionWindow;
		bool activeCreateDatabaseWindow;
		bool activeCreateTableWindow;
		bool activeCreateRelationWindow;
		bool activeCreateConditionWindow;
		bool activeDebugWindow;
		bool activeScriptManager;

		ImGuiDatabaseWindow dbWindow;
		ImGuiDatabaseTableView tblWindow;
		ImGuiRecordWindow recWindow;
		ImGuiRelationalWindow relWindow;
		ImGuiConditionWindow cndWindow;
		ImGuiCreateDatabaseWindow cdbWindow;
		ImGuiCreateTableWindow ctblWindow;
		ImGuiCreateRelationalWindow crelWindow;
		ImGuiCreateConditionWindow ccndWindow;
		ImGuiDebugWindow dbgWindow;
		ImGuiScriptManager scriptManager;
		MugenScript::MugenScriptDatabaseEditor database;
		MugenScript::MugenScriptDatabaseEditor::table_t* table;
		Mugenshuen::array_t<MugenScript::MugenScriptDatabaseValue> dstTableWindowRecord;
		Mugenshuen::array_t<MugenScript::MugenScriptDatabaseValue> dstRecordWindowEdit;
		Mugenshuen::array_t<MugenScript::MugenScriptDatabaseValue> dstTableWindowColumnInitial;
		Mugenshuen::array_t<MugenScript::DATABASE_VALUE_TYPE> dstTableWindowColumnType;
		Mugenshuen::array_t<Mugenshuen::array_t<MugenScript::MugenScriptDatabaseValue>> dstRelationalWindowRecord;
	};
	
}