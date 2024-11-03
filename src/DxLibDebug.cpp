#include <Windows.h>
#include "DxLibDebug.h"
#include "common/FileSystem.h"




int MugenScriptDebug::MugenScriptProcess::ExecuteProcess(wchar_t* processPath, wchar_t* dir)
{
	STARTUPINFO startUpInfo;
	ZeroMemory(&procInfo, sizeof(procInfo));
	ZeroMemory(&startUpInfo, sizeof(startUpInfo));
	GetStartupInfo(&startUpInfo);

	auto result = CreateProcess(
		processPath,
		0,
		0,
		0,
		FALSE,
		CREATE_NEW_PROCESS_GROUP,
		0,
		0,
		&startUpInfo,
		&procInfo
	);

	if (!result)
	{
		LPDWORD exitCode=0;
		GetExitCodeProcess(&procInfo, exitCode);

		//assert(false);
		return -1;
	}

	//WaitForSingleObject(procInfo.hProcess, INFINITE);
	//CloseHandle(procInfo.hProcess);

	return 0;
}

bool MugenScriptDebug::MugenScriptProcess::FinishedProcess()
{
	DWORD ret = 0;
	auto result = GetExitCodeProcess(procInfo.hProcess, &ret);
	return result != 0 && ret != STILL_ACTIVE;
}

bool MugenScriptDebug::MugenScriptProcess::CloseProcess()
{
	if (!FinishedProcess())
		return false;
	CloseHandle(procInfo.hProcess);
	return true;
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::InsertDebugValue(Mugenshuen::string_t key, MugenScript::DATABASE_VALUE_TYPE type, MugenScriptDatabaseDebugValueType io)
{
	MugenScriptDatabaseDebugData data;
	data.name = key;
	data.type = type;
	data.ioType = io;
	values.insert(data);
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::InsertDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue src, MugenScriptDatabaseDebugValueType ioType)
{
	MugenScriptDatabaseDebugData data;
	data.name = key;
	data.type = src.type;
	data.ioType = ioType;
	data.value = src;
	values.insert(data);
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::RemoveDebugValue(Mugenshuen::string_t key)
{
	MugenScriptDatabaseDebugData data;
	data.name = key;
	values.remove(data);
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::StartProcess(wchar_t* processPath)
{
	if (!IsExecutable(processPath))
		return;

	InsertDebugValue("com_count", MugenScript::DATABASE_VALUE_TYPE_INT);
	InsertDebugValue("message_count", MugenScript::DATABASE_VALUE_TYPE_INT);
	InsertDebugValue("work_place", MugenScript::DATABASE_VALUE_TYPE_STR);
	WriteProperty();
	FileSystem::path_t absPath = FileSystem::Absolute(processPath);
	process.ExecuteProcess(absPath.wstring().data());
	mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, L"MugenScriptDebug");
}

bool MugenScriptDebug::MugenScriptProcessDebuggerMaster::IsFinishedProcess()
{
	return process.FinishedProcess();
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::EndProcess()
{
	int counter = 0;
	while (!process.CloseProcess()&&counter<255)
	{
		std::this_thread::yield();
		counter++;
	}
	Clear();
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::UpdateDebuggerProcess()
{
	if (!messageQueue.empty())
	{
		auto pBuf = Mapping(FILE_MAP_WRITE);
		if (pBuf)
		{
			auto pWrite = (int*)&pBuf[reservePos];
			bool isWritable = *pWrite & MugenScriptDebugProcessMessage_Writable;
			if (isWritable)
			{
				*pWrite &= ~(MugenScriptDebugProcessMessage_Writable | MugenScriptDebugProcessMessage_Readable);
				auto message = messageQueue.pop();
				pWrite[0] = message.message;
				pWrite[1] = message.pos;
				*pWrite |= MugenScriptDebugProcessMessage_Readable;
			}
		}
	}
}
int MugenScriptDebug::MugenScriptProcessDebuggerMaster::GetCommunication()
{
	return GetDebugValue("com_count").value.ivalue;
}
int MugenScriptDebug::MugenScriptProcessDebuggerMaster::GetMessageCounter()
{
	return GetDebugValue("message_count").value.ivalue;
}

int MugenScriptDebug::MugenScriptProcessDebuggerMaster::GetReservePos()
{
	return GetDebugValue("reserve_pos").value.ivalue;
}

Mugenshuen::string_t MugenScriptDebug::MugenScriptProcessDebuggerMaster::GetProcessWorkPlace()
{
	return GetDebugValue("work_place").value.string;
}


void MugenScriptDebug::MugenScriptProcessDebuggerMaster::MugenScriptDatabaseDebugData::operator=(const MugenScriptDatabaseDebugData& o)
{
	name = o.name;
	type = o.type;
	value = o.value;
	ioType = o.ioType;
	pos = o.pos;
}

MugenScript::MugenScriptDatabaseValue MugenScriptDebug::MugenScriptProcessDebuggerMaster::GetDebugValue(Mugenshuen::string_t key)
{
	int keyIdx = 0;
	LPCSTR pBuf;
	MugenScriptDatabaseDebugData data;
	MugenScript::MugenScriptDatabaseValue ret(0);
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	data.name = key;
	auto n = values.find(keyIdx,data);
	if (!n)
		return MugenScript::MugenScriptDatabaseValue(0);

	if (mapping == NULL)
		return MugenScript::MugenScriptDatabaseValue(0);

	pBuf = (LPSTR)Mapping(FILE_MAP_READ);
	if (pBuf)
	{
		memio.ReadValue(n->keys[keyIdx].type, ret, &pBuf[n->keys[keyIdx].pos]);
		Unmap(pBuf);
	}
	return ret;
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::SetDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue value)
{
	int keyIdx = 0;
	LPSTR pBuf;
	MugenScriptDatabaseDebugData data;
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	data.name = key;
	auto n = values.find(keyIdx, data);
	if (!n)
		return ;
	pBuf = (LPSTR)Mapping(FILE_MAP_WRITE);
	if (pBuf)
	{
		int pos = n->keys[keyIdx].pos;
		memio.WriteValue(&pBuf[pos], bufferSize-pos, &value);

		MugenScriptProcessMessage message = {};
		message.pos = pos;
		message.message = MugenScriptDebugProcessMessage_SetValue;
		messageQueue.push(message);

		Unmap(pBuf);
	}
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::Clear()
{
	bufferSize = 0;
	mapSize = 0;
	CloseHandle(mapping);
	process.CloseProcess();
	values.clear();
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::WriteProperty()
{
	LPSTR pBuf;
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	bufferSize = sizeof(int) * 4;
	int mapSize = 0;
	for (int i = 0; i < values.size(); ++i)
	{
		auto& n = values[i];
		n.pos = bufferSize;

		mapSize += n.name.Lengh() + 1;
		mapSize += sizeof(n.type);
		mapSize += sizeof(n.pos);
		switch (n.type)
		{
		case MugenScript::DATABASE_VALUE_TYPE_INT:
			bufferSize += sizeof(int);
			break;

		case MugenScript::DATABASE_VALUE_TYPE_DBL:
			bufferSize += sizeof(double);
			break;

		case MugenScript::DATABASE_VALUE_TYPE_STR:
			bufferSize += MUGEN_SCRIPT_DEBUG_SHARED_MEM_MAX_STR * sizeof(wchar_t);
			break;
		default:
			break;
		}
	}
	for (int i = 0; i < values.size(); ++i)
		values[i].pos += mapSize;

	mapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize+mapSize+sizeof(int), L"MugenScriptDebug");
	if (!mapping)
		return;
	pBuf = (LPSTR)Mapping(FILE_MAP_ALL_ACCESS);
	assert(pBuf);
	int idx = 0;
	MugenScript::MugenScriptDatabaseValue vNum(values.size());
	MugenScript::MugenScriptDatabaseValue vBuffSize(bufferSize);
	MugenScript::MugenScriptDatabaseValue vReserve1(MugenScriptDebugProcessMessage_Writable);
	MugenScript::MugenScriptDatabaseValue vReserve2(0);
	idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vNum);
	idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vBuffSize);
	reservePos = idx;
	idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vReserve1);
	idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vReserve2);

	for (int i = 0; i < values.size(); ++i)
	{
		auto& n = values[i];
		MugenScript::MugenScriptDatabaseValue vName(n.name.C_Str());
		MugenScript::MugenScriptDatabaseValue vType((int)n.type);
		MugenScript::MugenScriptDatabaseValue vPos((int)n.pos);
		idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vName);
		idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vType);
		idx += memio.WriteValue(&pBuf[idx], bufferSize - idx, &vPos);

		if (n.ioType == MugenScriptDatabaseDebugValueType_Out)
			memio.WriteValue(&pBuf[n.pos], bufferSize - n.pos, &n.value);
	}
	UnmapViewOfFile(pBuf);
}

bool MugenScriptDebug::MugenScriptProcessDebuggerMaster::IsExecutable(wchar_t* processPath)
{
	Mugenshuen::string_t path;
	auto len = wcslen(processPath);
	auto exist = FileSystem::Exists(processPath);
	auto extent = FileSystem::Extension(processPath);
	if (exist && extent == L".exe")
		return true;
	return false;
}

LPSTR MugenScriptDebug::MugenScriptProcessDebuggerMaster::Mapping(DWORD io)
{
	return (LPSTR)MapViewOfFile(mapping, io, 0, 0, bufferSize);
}

void MugenScriptDebug::MugenScriptProcessDebuggerMaster::Unmap(LPCSTR p)
{
	if (p)
		UnmapViewOfFile(p);
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::SetProperty(MugenScriptDebug::MugenScriptProcessDebuggerMaster* debugger)
{
	debugger->InsertDebugValue("debugger_pc_out", MugenScript::DATABASE_VALUE_TYPE_INT);
	debugger->InsertDebugValue("debugger_page_out", MugenScript::DATABASE_VALUE_TYPE_INT);
	debugger->InsertDebugValue("debugger_line_out", MugenScript::DATABASE_VALUE_TYPE_INT);

	debugger->InsertDebugValue("debugger_pc_in", MugenScript::MugenScriptDatabaseValue(0));
	debugger->InsertDebugValue("debugger_page_in", MugenScript::MugenScriptDatabaseValue(0));
	debugger->InsertDebugValue("debugger_line_in", MugenScript::MugenScriptDatabaseValue(0));

	debugger->InsertDebugValue("debugger_script_path", MugenScript::DATABASE_VALUE_TYPE_STR);
	debugger->InsertDebugValue("debugger_execute", MugenScript::DATABASE_VALUE_TYPE_INT);
	debugger->InsertDebugValue("machine_pc", MugenScript::DATABASE_VALUE_TYPE_INT);
	debugger->InsertDebugValue("machine_prog_size", MugenScript::DATABASE_VALUE_TYPE_INT);

	this->debugger = debugger;
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::SetPC(int pc)
{
	debugger->SetDebugValue("debugger_pc_out", pc);
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::SetPage(int page)
{
	debugger->SetDebugValue("debugger_page_out", page);
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::SetLine(int line)
{
	debugger->SetDebugValue("debugger_line_out", line);
}

int MugenScriptDxLib::MugenScriptDxLibMachineDebugger::GetPC()
{
	return debugger->GetDebugValue("machine_pc").value.ivalue;
}

int MugenScriptDxLib::MugenScriptDxLibMachineDebugger::GetPage()
{
	return debugger->GetDebugValue("debugger_page_in").value.ivalue;
}

int MugenScriptDxLib::MugenScriptDxLibMachineDebugger::GetLine()
{
	return debugger->GetDebugValue("debugger_line_in").value.ivalue;
}

int MugenScriptDxLib::MugenScriptDxLibMachineDebugger::GetProgSize()
{
	return debugger->GetDebugValue("machine_prog_size").value.ivalue;
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::LoadScript(Mugenshuen::string_t path)
{
	assert(path.Lengh() < MUGEN_SCRIPT_DEBUG_SHARED_MEM_MAX_STR);
	debugger->SetDebugValue("debugger_script_path", path.C_Str());
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::ExecutePage()
{
	debugger->SetDebugValue("debugger_execute", MugenScriptDxLibDebuggerExecute_Page);
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::ExecuteLine()
{
	debugger->SetDebugValue("debugger_execute", MugenScriptDxLibDebuggerExecute_Line);
}

void MugenScriptDxLib::MugenScriptDxLibMachineDebugger::ExecutePC()
{
	debugger->SetDebugValue("debugger_execute", MugenScriptDxLibDebuggerExecute_PC);
}

void MugenScriptDebug::MugenScriptProcessMessageQueue::push(const MugenScriptProcessMessage message)
{
	messageQueue.push(message);
}

MugenScriptDebug::MugenScriptProcessMessage MugenScriptDebug::MugenScriptProcessMessageQueue::pop()
{
	auto ret = messageQueue.front();
	messageQueue.pop();
	return ret;
}

bool MugenScriptDebug::MugenScriptProcessMessageQueue::empty() const
{
	return messageQueue.empty();
}
