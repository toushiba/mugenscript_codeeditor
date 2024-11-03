#pragma once
#include "MugenScriptDatabase.h"
#include "common/JobSystem.h"
#define MUGEN_SCRIPT_DEBUG_SHARED_MEM_MAX_STR 256

namespace MugenScriptDebug
{
	enum MugenScriptDatabaseDebugValueType_
	{
		MugenScriptDatabaseDebugValueType_In = 1 << 0,
		MugenScriptDatabaseDebugValueType_Out = 1 << 1
	};

	using MugenScriptDatabaseDebugValueType = unsigned int;

	class MugenScriptProcess
	{
	public:

		int ExecuteProcess(wchar_t* processPath,wchar_t* dir=nullptr);
		bool FinishedProcess();
		bool CloseProcess();
	private:

		PROCESS_INFORMATION procInfo;
	};

	struct MugenScriptProcessMessage
	{
		int message;
		int pos;
	};

	class MugenScriptProcessMessageQueue
	{
	public:

		void push(const MugenScriptProcessMessage message);
		MugenScriptProcessMessage pop();
		bool empty()const;

	private:
		Mugenshuen::queue_t<MugenScriptProcessMessage> messageQueue;
	};

	class MugenScriptProcessDebuggerMaster
	{
	public:


		enum MugenScriptDebugProcessMessage_
		{
			MugenScriptDebugProcessMessage_None = 0,
			MugenScriptDebugProcessMessage_SetValue = 1 << 0,
			MugenScriptDebugProcessMessage_Readable = 1 << 30,
			MugenScriptDebugProcessMessage_Writable = 1 << 31
		};

		struct MugenScriptDatabaseDebugData
		{
			Mugenshuen::string_t name;
			MugenScript::DATABASE_VALUE_TYPE type;
			MugenScript::MugenScriptDatabaseValue value;
			MugenScriptDatabaseDebugValueType ioType;
			int pos;
			bool operator<(const MugenScriptDatabaseDebugData& o)const { return name < o.name; };
			bool operator>(const MugenScriptDatabaseDebugData& o)const { return name > o.name; };
			bool operator<=(const MugenScriptDatabaseDebugData& o)const { return name <= o.name; };
			bool operator>=(const MugenScriptDatabaseDebugData& o)const { return name >= o.name; };
			bool operator==(const MugenScriptDatabaseDebugData& o)const { return name == o.name; };
			void operator=(const MugenScriptDatabaseDebugData& o);

		};
		void InsertDebugValue(Mugenshuen::string_t key, MugenScript::DATABASE_VALUE_TYPE type, MugenScriptDatabaseDebugValueType io = MugenScriptDatabaseDebugValueType_Out);
		void InsertDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue src, MugenScriptDatabaseDebugValueType ioType = MugenScriptDatabaseDebugValueType_In);
		void RemoveDebugValue(Mugenshuen::string_t key);
		void StartProcess(wchar_t* processPath);
		bool IsFinishedProcess();
		void EndProcess();
		void UpdateDebuggerProcess();
		int GetCommunication();
		int GetMessageCounter();
		int GetReservePos();
		Mugenshuen::string_t GetProcessWorkPlace();

		MugenScript::MugenScriptDatabaseValue GetDebugValue(Mugenshuen::string_t key);
		void SetDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue value);
	private:
		void Clear();
		void WriteProperty();
		bool IsExecutable(wchar_t* processPath);
		LPSTR Mapping(DWORD io);
		void Unmap(LPCSTR p);
		int bufferSize;
		int mapSize;
		int reservePos;
		HANDLE mapping;
		HANDLE data;
		MugenScriptProcess process;
		MugenScriptProcessMessageQueue messageQueue;
		Mugenshuen::btree_t<MugenScriptDatabaseDebugData> values;
	};

}

namespace MugenScriptDxLib
{
	enum MugenScriptDxLibDebuggerExecute_
	{
		MugenScriptDxLibDebuggerExecute_Page,
		MugenScriptDxLibDebuggerExecute_Line,
		MugenScriptDxLibDebuggerExecute_PC,
	};

	class MugenScriptDxLibMachineDebugger
	{
	public:

		void SetProperty(MugenScriptDebug::MugenScriptProcessDebuggerMaster* debugger);
		
		void SetPC(int pc);
		void SetPage(int page);
		void SetLine(int line);
		int GetPC();
		int GetPage();
		int GetLine();
		int GetProgSize();

		void LoadScript(Mugenshuen::string_t path);
		void ExecutePage();
		void ExecuteLine();
		void ExecutePC();

		

	private:
		MugenScriptDebug::MugenScriptProcessDebuggerMaster* debugger;
	};

}
