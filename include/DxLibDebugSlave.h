#pragma once
#include <Windows.h>
#include "MugenScriptDatabase.h"

namespace MugenScriptDebug
{
	class MugenScriptDxLibDebuggerSlave
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
			int ioType;
			int pos;
			bool operator<(const MugenScriptDatabaseDebugData& o)const { return name < o.name; };
			bool operator>(const MugenScriptDatabaseDebugData& o)const { return name > o.name; };
			bool operator<=(const MugenScriptDatabaseDebugData& o)const { return name <= o.name; };
			bool operator>=(const MugenScriptDatabaseDebugData& o)const { return name >= o.name; };
			bool operator==(const MugenScriptDatabaseDebugData& o)const { return name == o.name; };
			void operator=(const MugenScriptDatabaseDebugData& o);

		};
		struct MessageData
		{
			int message;
			int pos;
			Mugenshuen::string_t name;
		};

		struct PosMapData
		{
			int pos;
			Mugenshuen::string_t name;
			operator int()const { return pos; }
			void operator=(int i) { pos = i; }
		};
		using DebugData = MugenScriptDatabaseDebugData;

		int StartDebugging(LPCTSTR);
		void UpdateDebuggerProcess();
		MessageData PopMessage();
		bool IsEmptyMessage();

		MugenScript::MugenScriptDatabaseValue GetDebugValue(Mugenshuen::string_t key);
		void SetDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue value);

	private:

		LPSTR Mapping(DWORD io);
		void Unmap(LPCSTR p);

		int messageCounter;
		int communicationCounter;
		int reservePos;
		int bufferSize;
		HANDLE handle;
		Mugenshuen::btree_t<DebugData> map;
		Mugenshuen::btree_t<PosMapData> posMap;
		Mugenshuen::queue_t<MessageData> messageQueue;
	};
}