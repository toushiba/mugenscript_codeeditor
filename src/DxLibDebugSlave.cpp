#include "DxLibDebugSlave.h"

int MugenScriptDebug::MugenScriptDxLibDebuggerSlave::StartDebugging(LPCTSTR mapName)
{
	handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mapName);
	if (handle == NULL)
		return -1;
	LPSTR pBuf;
	pBuf = (LPSTR)MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (pBuf == NULL)
		return -1;
	int idx = 0;
	LPCSTR read = &pBuf[idx];
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScript::MugenScriptDatabaseValue num;
	MugenScript::MugenScriptDatabaseValue buffSize;
	MugenScript::MugenScriptDatabaseValue reserve1;
	MugenScript::MugenScriptDatabaseValue reserve2;
	read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, num, read);
	read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, buffSize, read);
	reservePos = read - pBuf;
	read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, reserve1, read);
	read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, reserve2, read);
	bufferSize = buffSize.value.ivalue;
	for (int i = 0; i < num.value.ivalue; ++i)
	{
		DebugData data;
		MugenScript::MugenScriptDatabaseValue name;
		MugenScript::MugenScriptDatabaseValue type;
		MugenScript::MugenScriptDatabaseValue pos;
		read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_STR, name, read);
		read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, type, read);
		read = memio.ReadValue(MugenScript::DATABASE_VALUE_TYPE_INT, pos, read);
		data.name = name.value.string;
		data.type = (MugenScript::DATABASE_VALUE_TYPE)type.value.ivalue;
		data.pos = pos.value.ivalue;
		map.insert(data);

		PosMapData posData;
		posData.name = name.value.string;
		posData.pos = pos.value.ivalue;
		posMap.insert(posData);
	}
	return 0;
}

void MugenScriptDebug::MugenScriptDxLibDebuggerSlave::UpdateDebuggerProcess()
{
	auto pBuf = Mapping(FILE_MAP_ALL_ACCESS);
	if (!pBuf)
		return;
	auto read = (int*)&pBuf[reservePos];
	bool isReadable = *read & MugenScriptDebugProcessMessage_Readable;
	if (isReadable)
	{
		*read &= ~(MugenScriptDebugProcessMessage_Readable | MugenScriptDebugProcessMessage_Writable);
		MessageData data;
		data.message = read[0];
		data.pos = read[1];
		*read |= MugenScriptDebugProcessMessage_Writable;

		int keyIdx = 0;
		PosMapData posMapData;
		posMapData = data.pos;
		auto n = posMap.find(keyIdx, posMapData);
		data.name = n ? n->keys[keyIdx].name : "";
		messageQueue.push(data);
		messageCounter++;
		SetDebugValue("message_count", messageCounter);
		
	}
	SetDebugValue("com_count", communicationCounter++);
	SetDebugValue("reserve_pos", reservePos);
	SetDebugValue("work_place", FileSystem::Current().string().c_str());
	if (communicationCounter > 256)
		communicationCounter = 0;
}

MugenScriptDebug::MugenScriptDxLibDebuggerSlave::MessageData MugenScriptDebug::MugenScriptDxLibDebuggerSlave::PopMessage()
{
	auto ret = messageQueue.front();
	messageQueue.pop();
	return ret;
}

bool MugenScriptDebug::MugenScriptDxLibDebuggerSlave::IsEmptyMessage()
{
	return messageQueue.empty();
}

void MugenScriptDebug::MugenScriptDxLibDebuggerSlave::MugenScriptDatabaseDebugData::operator=(const MugenScriptDatabaseDebugData& o)
{
	name = o.name;
	type = o.type;
	value = o.value;
	ioType = o.ioType;
	pos = o.pos;
}


MugenScript::MugenScriptDatabaseValue MugenScriptDebug::MugenScriptDxLibDebuggerSlave::GetDebugValue(Mugenshuen::string_t key)
{
	int keyIdx = 0;
	LPCSTR pBuf;
	DebugData data;
	MugenScript::MugenScriptDatabaseValue ret(0);
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	data.name = key;
	auto n = map.find(keyIdx, data);
	if (!n)
		return MugenScript::MugenScriptDatabaseValue(0);

	if (handle == NULL)
		return MugenScript::MugenScriptDatabaseValue(0);

	pBuf = (LPSTR)Mapping(FILE_MAP_READ);
	if (pBuf)
	{
		memio.ReadValue(n->keys[keyIdx].type, ret, &pBuf[n->keys[keyIdx].pos]);
		Unmap(pBuf);
	}
	return ret;
}

void MugenScriptDebug::MugenScriptDxLibDebuggerSlave::SetDebugValue(Mugenshuen::string_t key, MugenScript::MugenScriptDatabaseValue value)
{
	int keyIdx = 0;
	LPSTR pBuf;
	DebugData data;
	MugenScript::MugenScriptDatabaseTableDataMemWriterReader memio;
	data.name = key;
	auto n = map.find(keyIdx, data);
	if (!n)
		return;
	pBuf = (LPSTR)Mapping(FILE_MAP_WRITE);
	if (pBuf)
	{
		int pos = n->keys[keyIdx].pos;
		memio.WriteValue(&pBuf[pos], bufferSize - pos, &value);
		Unmap(pBuf);
	}
}

LPSTR MugenScriptDebug::MugenScriptDxLibDebuggerSlave::Mapping(DWORD io)
{
	return (LPSTR)MapViewOfFile(handle, io, 0, 0, bufferSize);
}

void MugenScriptDebug::MugenScriptDxLibDebuggerSlave::Unmap(LPCSTR p)
{
	UnmapViewOfFile(p);
}
