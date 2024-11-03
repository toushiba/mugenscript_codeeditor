#include "MugenScriptDatabase.h"
#define MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE 8

bool MugenScript::MugenScriptDatabaseValue::operator<=(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue <= o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue <= o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) <= 0; };

	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}

bool MugenScript::MugenScriptDatabaseValue::operator>=(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue >= o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue >= o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) >= 0; };
	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}

bool MugenScript::MugenScriptDatabaseValue::operator<(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue < o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue < o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) < 0; };
	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}

bool MugenScript::MugenScriptDatabaseValue::operator>(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue > o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue > o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) > 0; };
	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}

bool MugenScript::MugenScriptDatabaseValue::operator==(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue == o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue == o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) == 0; };
	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}

bool MugenScript::MugenScriptDatabaseValue::operator!=(const MugenScriptDatabaseValue& o)const
{
	auto ifunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.ivalue != o.value.ivalue; };
	auto dfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return t.value.dvalue != o.value.dvalue; };
	auto sfunc = [](const MugenScriptDatabaseValue& t, const MugenScriptDatabaseValue& o) {return strcmp(t.value.string, o.value.string) != 0; };
	return QueryDataTypeFunctionCompare(o, ifunc, dfunc, sfunc);
}


void MugenScript::MugenScriptDatabaseValue::operator=(const MugenScriptDatabaseValue& value)
{
	if (type == DATABASE_VALUE_TYPE_STR && this->value.string)
	{
		delete[] this->value.string;
		this->value.string = nullptr;
	}
	type = value.type;
	userData = value.userData;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		this->value.ivalue = value.value.ivalue;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		this->value.dvalue = value.value.dvalue;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		if (!value.value.string)
			return;
		auto len = strlen(value.value.string) + 1;
		this->value.string = new char[len];
		strcpy_s(this->value.string, len, value.value.string);
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		this->value.ivalue = value.value.ivalue;
		break;
	default:
		break;
	}
}

void MugenScript::MugenScriptDatabaseValue::operator=(MugenScriptDatabaseValue&& value)
{
	memcpy_s(&this->value, sizeof(TableValue) + 1, &value.value, sizeof(TableValue));
	memset(&value.value, 0, sizeof(TableValue));
	type = value.type;
	userData = value.userData;
	value.type = DATABASE_VALUE_TYPE_INT;
}

int MugenScript::MugenScriptDatabaseValue::size()const
{
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		return sizeof(int);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		return sizeof(double);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
		return strlen(value.string) + 1;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		return sizeof(int);
		break;
	default:
		//return userDataSize+sizeof(int);
		break;
	}
	return 0;
}

MugenScript::MugenScriptDatabaseValue::MugenScriptDatabaseValue(const char* string) :type(DATABASE_VALUE_TYPE_STR), value(),userData()
{
	auto len = strlen(string) + 1;
	this->value.string = new char[len];
	strcpy_s(this->value.string, len, string);
}

MugenScript::MugenScriptDatabaseValue::MugenScriptDatabaseValue(DATABASE_VALUE_TYPE type) :type(type), value(0.0),userData()
{
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		*this = 0;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		*this = 0.0;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
		*this = "";
		break;
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		value.ivalue = -1;
		break;
	default:
		break;
	}
}

MugenScript::MugenScriptDatabaseValue::MugenScriptDatabaseValue(const MugenScriptDatabaseValue& value)
{
	*this = value;
}

MugenScript::MugenScriptDatabaseValue::MugenScriptDatabaseValue(MugenScriptDatabaseValue&& value)
{
	*this = value;
}

MugenScript::MugenScriptDatabaseValue::~MugenScriptDatabaseValue()
{
	if (type == DATABASE_VALUE_TYPE_STR && value.string)
		delete[] value.string;
}

bool MugenScript::MugenScriptDatabaseValue::QueryDataTypeFunctionCompare(
	const MugenScriptDatabaseValue& o,
	bool(*ifunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&),
	bool(*dfunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&),
	bool(*sfunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&)
)const
{
	assert(type == o.type);
	switch (o.type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		return ifunc(*this, o);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		return dfunc(*this, o);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
		return sfunc(*this, o);
		break;
	default:
		assert(false);
		break;
	}
	return false;
}

int MugenScript::MugenScriptDatabaseTableDataMemWriterReader::WriteValue(char* dst, int buffDst, const MugenScriptDatabaseValue* value)const
{
	auto type = value->type;
	char* write = dst;
	int size = 0;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		write = WriteMem(write, buffDst, &value->value.ivalue, sizeof(int));
		size = value->size();
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		write = WriteMem(write, buffDst, &value->value.dvalue, sizeof(double));
		size = value->size();
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		int len = value->size();
		if (value->value.string)
			write = WriteMem(write, buffDst, value->value.string, len);
		size = len;
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		write = WriteMem(write, buffDst, &value->value.ivalue, sizeof(int));
		size = value->size();
		break;
	default:
		//write = WriteMem(write, buffDst, &value->userDataSize, sizeof(value->userDataSize));
		//write = WriteMem(write, buffDst, value->value.userData, value->userDataSize);
		//size = sizeof(int) + value->userDataSize;
		break;
	}

	return size;
}

const char* MugenScript::MugenScriptDatabaseTableDataMemWriterReader::ReadValue(DATABASE_VALUE_TYPE type, MugenScriptDatabaseValue& dst, const char* src)const
{
	int size = 0;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		dst = *(int*)src;
		size = sizeof(int);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		dst = *(double*)src;
		size = sizeof(double);
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		int len = strlen(src) + 1;
		dst = (const char*)src;
		size = len;
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		dst.type = DATABASE_VALUE_TYPE_INVALID;
		dst.value.ivalue = -1;
		size = sizeof(int);
		break;
	default:
	{
		size = sizeof(int);
		//dst.type = type;
		//dst.userDataSize = *(int*)src;
		//dst.value.userData = (void*)&src[sizeof(int)];
		//size = sizeof(int) + dst.userDataSize;
	}
	break;
	}

	return &src[size];
}

char* MugenScript::MugenScriptDatabaseTableDataMemWriterReader::WriteMem(char* dst, int buffDst, const void* src, int size)const
{
	memcpy_s(dst, buffDst, src, size);
	return &((char*)dst)[size];
}

int MugenScript::MugenScriptDatabaseEditor::CreateDatabase(Mugenshuen::string_t name, DATABASE_PAGE_SIZE page)
{
	Mugenshuen::string_t fileName = name + ".mdb";
	MugenScriptDatabaseHandle newHandle = {};
	if (file.exist(fileName))
		return -1;
	newHandle.pageSize = page;
	newHandle.numTable = 0;
	newHandle.mapStart = 0;
	newHandle.size = 0;
	file.open(fileName);
	file.write(&newHandle, sizeof(newHandle));
	file.close();
	return 0;
}

void MugenScript::MugenScriptDatabaseEditor::DeleteDatabase(Mugenshuen::string_t name)
{
	auto fileName = name + ".mdb";
	if (file.exist(fileName))
		file.remove(fileName);
}

void MugenScript::MugenScriptDatabaseEditor::LoadDatabase(Mugenshuen::string_t name)
{
	Mugenshuen::string_t fileName = name + ".mdb";
	assert(file.exist(fileName));
	database = name;
	file.open(fileName);
	file.read(&handle, sizeof(handle));
	ReadTableMap();
	for (int i = 0; i < tableMap.size(); ++i)
		positionMap.insert(tableMap[i].pos);
}

void MugenScript::MugenScriptDatabaseEditor::CloseDatabsae(bool save)
{
	CloseTable(save);
	if (file.is_loaded())
	{
		if (save)
			WriteDatabaseHandle();
		file.close();
	}

	database = "";
	handle = MugenScriptDatabaseHandle();
	editTables.clear();
	tableMap.clear();
	positionMap.clear();

}

MugenScript::MugenScriptDatabaseEditor::table_t* MugenScript::MugenScriptDatabaseEditor::LoadTable(Mugenshuen::string_t name)
{
	MugenScriptDatabaseTableMapData map;
	map.name = name;
	int keyIdx = 0;
	auto n = tableMap.find(keyIdx, map);

	if (!n)
		return nullptr;
	for (int i = 0; i < editTables.size(); ++i)
		if (editTables[i]->Name() == name)
			return editTables[i];

	auto tbl = new table_t();
	int pos = n->keys[keyIdx].pos;

	SeekTableStart(pos);
	tbl->Load(file);
	editTables.push_back(tbl);

	return tbl;
}

void MugenScript::MugenScriptDatabaseEditor::SaveTable()
{
	if (editTables.size() == 0)
		return;

	for (int i = 0; i < editTables.size(); ++i)
		SaveOneTable(editTables[i]);
	WriteTableMap();
	WriteDatabaseHandle();
}

void MugenScript::MugenScriptDatabaseEditor::SaveTable(table_t* table)
{
	auto it = std::find(editTables.begin(), editTables.end(), table);
	if (it == editTables.end())
		return;

	SaveOneTable(table);
	WriteTableMap();
	WriteDatabaseHandle();
}

void MugenScript::MugenScriptDatabaseEditor::CloseTable(bool save)
{
	if (save)
		SaveTable();
	for (int i = 0; i < editTables.size(); ++i)
		delete editTables[i];
	editTables.clear();
}

void MugenScript::MugenScriptDatabaseEditor::CloseTable(bool save, table_t* table)
{
	if (save)
		SaveTable(table);

	editTables.erase(std::remove(editTables.begin(), editTables.end(), table), editTables.end());
	delete table;
}

bool MugenScript::MugenScriptDatabaseEditor::FindTable(const Mugenshuen::string_t name)
{
	MugenScriptDatabaseTableMapData map;
	map.name = name;
	return tableMap.find(map);
}

bool MugenScript::MugenScriptDatabaseEditor::FindTable(const int pos)
{
	return positionMap.find(pos);
}

int MugenScript::MugenScriptDatabaseEditor::NumTable()
{
	return tableMap.size();
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseEditor::NameTable(int idx)
{
	return tableMap[idx].name;
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseEditor::CurrentDatabase()
{
	return database;
}



void MugenScript::MugenScriptDatabaseEditor::UpdateDatabase()
{
	for (int i = 0; i < editTables.size(); ++i)
	{
		MugenScriptDatabaseTableMapData map;
		int keyIdx = 0;
		map.name = editTables[i]->Name();
		auto n = tableMap.find(keyIdx, map);
		if (n)
		{
			auto wirteSize = editTables[i]->SizeOfDeploymentTable();

		}
	}
}

bool MugenScript::MugenScriptDatabaseEditor::CreateTable(const Mugenshuen::string_t name, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	if (FindTable(name))
		return false;

	auto tbl = new table_t(database,name, handle.pageSize, numColumn, names, types, initials);
	editTables.push_back(tbl);
	CloseTable(true, tbl);
	return true;
}

void MugenScript::MugenScriptDatabaseEditor::DeleteTable(Mugenshuen::string_t name)
{
	int keyIdx = 0;
	MugenScriptDatabaseTableMapData map;
	map.name = name;
	auto n = tableMap.find(keyIdx, map);

	if (n)
	{
		int pos = n->keys[keyIdx].pos;
		tableMap.remove(map);
		positionMap.remove(pos);
	}

	for (int i = 0; i < editTables.size(); ++i)
	{
		if (editTables[i]->Name() == name)
			CloseTable(false, editTables[i]);
	}
	handle.numTable = tableMap.size();
	WriteDatabaseHandle();
	WriteTableMap();
}

void MugenScript::MugenScriptDatabaseEditor::WriteTableMap()
{
	MugenScriptDatabaseWriter writer;
	writer.WriteDatabaseMap(handle, tableMap, file);
}

void MugenScript::MugenScriptDatabaseEditor::WriteDatabaseHandle()
{
	if (!file.is_loaded())
		return;
	MugenScriptDatabaseWriter writer;
	writer.WriteDatabaseHandle(handle, file);
}

void MugenScript::MugenScriptDatabaseEditor::ReadTableMap()
{
	MugenScriptDatabaseReader reader;
	reader.ReadDatabaseMap(tableMap, file);
}

void MugenScript::MugenScriptDatabaseEditor::ReadTableMap(const Mugenshuen::string_t databaseName, Mugenshuen::btree_t<MugenScriptDatabaseTableMapData>& dst) const
{
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseFile dbFile;
	MugenScriptDatabaseReader reader;
	dbFile.open(databaseName + ".mdb");
	reader.ReadDatabaseMap(dst, dbFile);
	dbFile.close();
}

int MugenScript::MugenScriptDatabaseEditor::AlignOfExtentByte(int byteSize)
{
	return (byteSize + (handle.pageSize * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE)) & ((~(handle.pageSize * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE)) + 1);
}

void MugenScript::MugenScriptDatabaseEditor::SaveOneTable(table_t* table)
{
	MugenScriptDatabaseTableMapData map;
	int keyIdx = 0;
	map.name = table->Name();
	auto n = tableMap.find(keyIdx, map);
	int tableSize = AlignOfExtentByte(table->SizeOfDeploymentTable());

	if (!n)
	{
		map.pos = handle.size;
		tableMap.insert(map);
		positionMap.insert(map.pos);
		handle.size += tableSize;

		SeekTableStart(map.pos);
	}
	else
	{
		int pos = n->keys[keyIdx].pos;
		int duplicateTables = positionMap.count_equal(pos + 1, pos + tableSize-1);
		if (duplicateTables)
		{
			n->keys[keyIdx].pos = handle.size;
			positionMap.remove(pos);
			positionMap.insert(handle.size);
			pos = handle.size;
			handle.size += tableSize;
		}

		SeekTableStart(pos);
	}
	handle.mapStart = handle.size;
	handle.numTable = tableMap.size();
	table->Save(file);
}

int MugenScript::MugenScriptDatabaseEditor::SizeOfHeader()const
{
	return sizeof(handle);
}

void MugenScript::MugenScriptDatabaseEditor::SeekTableStart(int pos)
{
	auto dis = file.distance();
	file.seek_begin(SizeOfHeader() + pos);
}

void MugenScript::MugenScriptDatabaseRecord::UpdateColumnData()
{
	if (!column)
		return;
	int num = column->GetNumColumn();
	if (!num)
		return;
	if (value)
		delete[] value;
	value = new MugenScriptDatabaseValue[num];
	this->column = column;
	this->numColumn = num;
}

void MugenScript::MugenScriptDatabaseRecord::Set(int id, int num, const MugenScriptDatabaseValue* value)
{
	this->id = id;
	if (value)
		for (int i = 0; i < numColumn; ++i)
			if (i < num)
				this->value[i] = value[i];
			else
				this->value[i] = column->GetInitial(i);
	else
		for (int i = 0; i < numColumn; ++i)
			this->value[i] = column->GetInitial(i);
}

MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseRecord::Get() const
{
	return value;
}

void MugenScript::MugenScriptDatabaseRecord::Delete(char* dst)
{
	auto write = &dst[sizeof(int)];
	*(int*)write = -1;
}

int MugenScript::MugenScriptDatabaseRecord::ID() const
{
	return id;
}

int MugenScript::MugenScriptDatabaseRecord::NumColumn() const
{
	return numColumn;
}

int MugenScript::MugenScriptDatabaseRecord::SizeOfDeployment()const
{
	int recordSize = 0;
	recordSize += sizeof(int);
	recordSize += sizeof(id);
	for (int i = 0; i < numColumn; ++i)
	{
		recordSize += value[i].size();
	}
	return recordSize;
}

int MugenScript::MugenScriptDatabaseRecord::DeployMemory(char* dst, int capacity)const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vSize(SizeOfDeployment());
	MugenScriptDatabaseValue vID(id);
	int idx = 0;

	idx += memio.WriteValue(&dst[idx], capacity - idx, &vSize);
	idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);

	for (int i = 0; i < numColumn; ++i)
		idx += memio.WriteValue(&dst[idx], capacity - idx, &value[i]);

	return idx;
}

int MugenScript::MugenScriptDatabaseRecord::CollectionMemory(const char* src)const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vSize(0);
	MugenScriptDatabaseValue vID(0);
	auto types = column->GetTypes();
	auto read = src;
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, read);
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);

	for (int i = 0; i < numColumn; ++i)
		read = memio.ReadValue(types[i], value[i], read);

	return read - src;
}

int MugenScript::MugenScriptDatabaseRecord::CollectionFile(MugenScriptDatabaseFile& file)const
{
	MugenScriptDatabaseTableDataFileReaderWriter reader;
	MugenScriptDatabaseValue vSize(0);
	MugenScriptDatabaseValue vID(0);
	int start = file.distance();
	auto types = column->GetTypes();
	reader.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, file);
	reader.ReadValue(DATABASE_VALUE_TYPE_INT, vID, file);

	for (int i = 0; i < numColumn; ++i)
		reader.ReadValue(types[i], value[i], file);

	return file.distance() - start;
}

int MugenScript::MugenScriptDatabaseRecord::CollectionSizeRecord(const char* src)const
{
	return *(const int*)src;
}

int MugenScript::MugenScriptDatabaseRecord::CollectionID(const char* src) const
{
	return *(int*)&src[sizeof(int)];
}

MugenScript::MugenScriptDatabaseRecord::MugenScriptDatabaseRecord(const MugenScriptDatabaseColumn* column) :
	numColumn(),
	id(),
	value(),
	column(column)
{
	UpdateColumnData();
}

MugenScript::MugenScriptDatabaseRecord::~MugenScriptDatabaseRecord()
{
	delete[]value;
}

void MugenScript::MugenScriptDatabaseColumn::operator=(const MugenScriptDatabaseColumn& o)
{
	numColumn = o.numColumn;

	this->type = new DATABASE_VALUE_TYPE[numColumn];
	this->name = new MugenScriptDatabaseValue[numColumn];
	this->initial = new MugenScriptDatabaseValue[numColumn];

	for (int i = 0; i < numColumn; ++i)
	{
		this->type[i] = o.type[i];
		this->name[i] = o.name[i];
		this->initial[i] = o.initial[i];
	}
}

void MugenScript::MugenScriptDatabaseColumn::operator=(MugenScriptDatabaseColumn&& o)
{
	if (type)
		delete[] type;
	if (name)
		delete[] name;
	if (initial)
		delete[] initial;

	numColumn = o.numColumn;
	type = o.type;
	name = o.name;
	initial = o.initial;
	o.numColumn = 0;
	o.type = nullptr;
	o.name = nullptr;
	o.initial = nullptr;
}

MugenScript::MugenScriptDatabaseColumn::MugenScriptDatabaseColumn(int numColumn, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* type, const MugenScriptDatabaseValue* initial) :
	numColumn(),
	type(),
	name(),
	initial()
{
	this->numColumn = numColumn;
	if (!numColumn)
		return;
	this->type = new DATABASE_VALUE_TYPE[numColumn];
	this->name = new MugenScriptDatabaseValue[numColumn];
	this->initial = new MugenScriptDatabaseValue[numColumn];

	for (int i = 0; i < numColumn; ++i)
	{
		assert(type[i] == initial[i].type);
		this->type[i] = type[i];
		this->name[i] = name[i].C_Str();
		this->initial[i] = initial[i];
	}
}

MugenScript::MugenScriptDatabaseColumn::MugenScriptDatabaseColumn(const MugenScriptDatabaseColumn& o)
{
	*this = o;
}

MugenScript::MugenScriptDatabaseColumn::MugenScriptDatabaseColumn(MugenScriptDatabaseColumn&& o)
{
	*this = std::move(o);
}

MugenScript::MugenScriptDatabaseColumn::~MugenScriptDatabaseColumn()
{
	Clear();
}

const MugenScript::DATABASE_VALUE_TYPE MugenScript::MugenScriptDatabaseColumn::GetType(int index)const
{
	return type[index];
}

const MugenScript::DATABASE_VALUE_TYPE* MugenScript::MugenScriptDatabaseColumn::GetTypes()const
{
	return type;
}

const MugenScript::MugenScriptDatabaseValue& MugenScript::MugenScriptDatabaseColumn::GetInitial(int index)const
{
	return initial[index];
}

const MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseColumn::GetInitials()const
{
	return initial;
}

const MugenScript::MugenScriptDatabaseValue& MugenScript::MugenScriptDatabaseColumn::GetName(int index)const
{
	return name[index];
}

const MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseColumn::GetNames()const
{
	return name;
}

int MugenScript::MugenScriptDatabaseColumn::Find(const Mugenshuen::string_t name) const
{
	for (int i = 0; i < numColumn; ++i)
		if (name == this->name[i].value.string)
			return i;
	return -1;
}

int MugenScript::MugenScriptDatabaseColumn::GetNumColumn()const
{
	return numColumn;
}

int MugenScript::MugenScriptDatabaseColumn::SizeOfDeployment()const
{
	int size = 0;
	size += sizeof(numColumn);
	size += sizeof(DATABASE_VALUE_TYPE) * numColumn;

	for (int i = 0; i < numColumn; ++i)
	{
		
		size += name[i].size();
		if (type[i] < DATABASE_VALUE_TYPE_NUM_TYPE)
			size += initial[i].size();
	}

	return size;
}

int MugenScript::MugenScriptDatabaseColumn::DeployMemory(char* dst, int capacity)const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vNumColumn(numColumn);
	int idx = 0;
	idx += memio.WriteValue(&dst[idx], capacity, &vNumColumn);

	for (int i = 0; i < numColumn; ++i)
	{
		MugenScriptDatabaseValue vType((int)type[i]);

		idx += memio.WriteValue(&dst[idx], capacity - idx, &vType);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &name[i]);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &initial[i]);
	}

	return idx;
}

int MugenScript::MugenScriptDatabaseColumn::CollectionMemory(const char* src)
{
	Clear();

	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vNumColumn(0);
	const char* read = src;

	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vNumColumn, read);
	numColumn = vNumColumn.value.ivalue;

	type = new DATABASE_VALUE_TYPE[numColumn];
	name = new MugenScriptDatabaseValue[numColumn];
	initial = new MugenScriptDatabaseValue[numColumn];

	for (int i = 0; i < numColumn; ++i)
	{
		MugenScriptDatabaseValue vType(0);
		Mugenshuen::string_t vName;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vType, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, name[i], read);
		read = memio.ReadValue((DATABASE_VALUE_TYPE)vType.value.ivalue, initial[i], read);

		type[i] = (DATABASE_VALUE_TYPE)vType.value.ivalue;
	}
	return read - src;
}

int MugenScript::MugenScriptDatabaseColumn::CollectionFile(MugenScriptDatabaseFile& file)
{
	Clear();

	MugenScriptDatabaseTableDataFileReaderWriter reader;
	MugenScriptDatabaseValue vNumColumn(0);
	auto start = file.distance();

	reader.ReadValue(DATABASE_VALUE_TYPE_INT, vNumColumn, file);
	numColumn = vNumColumn.value.ivalue;

	type = new DATABASE_VALUE_TYPE[numColumn];
	name = new MugenScriptDatabaseValue[numColumn];
	initial = new MugenScriptDatabaseValue[numColumn];

	for (int i = 0; i < numColumn; ++i)
	{
		MugenScriptDatabaseValue vType(0);
		Mugenshuen::string_t vName;
		reader.ReadValue(DATABASE_VALUE_TYPE_INT, vType, file);
		reader.ReadValue(DATABASE_VALUE_TYPE_STR, name[i], file);
		reader.ReadValue((DATABASE_VALUE_TYPE)vType.value.ivalue, initial[i], file);

		type[i] = (DATABASE_VALUE_TYPE)vType.value.ivalue;
	}

	return file.distance() - start;;
}

void MugenScript::MugenScriptDatabaseColumn::Clear()
{
	numColumn = 0;
	delete[]type;
	delete[]name;
	delete[]initial;

	type = nullptr;
	name = nullptr;
	initial = nullptr;
}

const Mugenshuen::string_t MugenScript::MugenScriptDatabaseTable::Name()const
{
	return prop->name;
}

int MugenScript::MugenScriptDatabaseTable::SizeOfDeploymentTable() const
{
	return prop->tableSize + index.SizeOfDeployment() + column.SizeOfDeployment() + columnIndex.SizeOfDeployment();
}

int MugenScript::MugenScriptDatabaseTable::Create(int num, const MugenScriptDatabaseValue* value)
{
	int id = prop->nextID;
	record.Set(prop->nextID, num, value);
	index.Create(prop->nextID, prop->tableSize);
	columnIndex.Create(id, value);
	int size = record.SizeOfDeployment();
	if (capacity < prop->tableSize + size)
		Realloc(prop->pageSize);

	prop->tableSize += record.DeployMemory(&page[prop->tableSize], capacity);
	prop->nextID++;
	prop->numRecord++;
	return id;
}

void MugenScript::MugenScriptDatabaseTable::Delete(int id)
{
	int pos = 0;
	index.Read(id, pos);
	if (pos == -1)
		return;
	record.CollectionMemory(&page[pos]);
	index.Delete(id);
	columnIndex.Delete(id,record.Get());
	record.Delete(&page[pos]);
	prop->numRecord--;

}

int MugenScript::MugenScriptDatabaseTable::Read(int id, MugenScriptDatabaseValue* dst)const
{
	int pos = 0;
	int numColumn = column.GetNumColumn();
	index.Read(id, pos);
	if (pos == -1)
		return -1;
	record.CollectionMemory(&page[pos]);
	auto src = record.Get();

	for (int i = 0; i < numColumn; ++i)
			dst[i] = src[i];
	return 0;
}

void MugenScript::MugenScriptDatabaseTable::Replace(int id, const MugenScriptDatabaseValue* value)
{
	int num = column.GetNumColumn();
	int pos = 0;
	index.Read(id,pos);
	if (pos == -1)
		return;
	record.Set(id, column.GetNumColumn(),value);
	index.Delete(id);
	columnIndex.Replace(id, value);
	record.Delete(&page[pos]);

	index.Create(id, prop->tableSize);
	int size = record.SizeOfDeployment();
	if (capacity < prop->tableSize + size)
		Realloc(prop->pageSize);

	prop->tableSize += record.DeployMemory(&page[prop->tableSize], capacity);
}

int MugenScript::MugenScriptDatabaseTableEx::CreateRelation(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, Mugenshuen::string_t relTableName, const Mugenshuen::string_t relationColumn)
{
	auto id = FindColumn(columnName);
	if (id == -1)
		return -1;
	auto type = TypesColumn()[id];
	if (type != DATABASE_VALUE_TYPE_EX_REL&& type != DATABASE_VALUE_TYPE_EX_TRIGGER && TypesColumn()[id] != DATABASE_VALUE_TYPE_INT)
	{
		return -1;
	}
	return ((MugenScriptDatabaseTableRelationEx*)features[id])->CreateRelation(id, relationColumn, databaseName, relTableName);
}

int MugenScript::MugenScriptDatabaseTableEx::ReplaceRelation(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, Mugenshuen::string_t relTableName, const Mugenshuen::string_t relationColumn)
{
	auto id = FindColumn(columnName);
	if (id == -1)
		return -1;
	auto type = TypesColumn()[id];
	if (type != DATABASE_VALUE_TYPE_EX_REL && type != DATABASE_VALUE_TYPE_EX_TRIGGER && TypesColumn()[id] != DATABASE_VALUE_TYPE_INT)
	{
		return -1;
	}
	((MugenScriptDatabaseTableRelationEx*)features[id])->ReplaceRelation(id, relationColumn, databaseName, relTableName);
	return 0;
}

int MugenScript::MugenScriptDatabaseTableEx::ReadRelational(int id,int columnID,int dstBuff, MugenScriptDatabaseValue* dst)
{
	return ((MugenScriptDatabaseTableRelationEx*)features[columnID])->ReadRelation(id, dstBuff, dst);
}

void MugenScript::MugenScriptDatabaseTableEx::DeleteRelation(const Mugenshuen::string_t columnName)
{
	auto id = FindColumn(columnName);
	if (!id)
		return;
	((MugenScriptDatabaseTableRelationEx*)features[id])->DeleteRelation();
}

MugenScript::MugenScriptDatabaseTableRelationEx::RelationData* MugenScript::MugenScriptDatabaseTableEx::GetRelation(int columnID) const
{
	return ((MugenScriptDatabaseTableRelationEx*)features[columnID])->Relation();
}

int MugenScript::MugenScriptDatabaseTable::CreateIndex(const Mugenshuen::string_t columnName)
{
	columnIndex.CreateColumnIndex(column.Find(columnName), index, page);
	return 0;
}

int MugenScript::MugenScriptDatabaseTable::ReadIndex(const Mugenshuen::string_t columnName, const MugenScriptDatabaseValue value, int dstBuf, int ids[]) const
{
	auto id = column.Find(columnName);
	columnIndex.Read(id, value,dstBuf, ids);
	return 0;
}

int MugenScript::MugenScriptDatabaseTable::NumColumnValue(const Mugenshuen::string_t columnName, const MugenScriptDatabaseValue value)const
{
	return columnIndex.Count(column.Find(columnName), value);
}


void MugenScript::MugenScriptDatabaseTable::Update()
{
	int numColumn = column.GetNumColumn();
	int idx = 0;
	int deleteRecord = 0;
	auto write = &page[sizeof(MugenScriptDatabaseTableProp)];
	index.Clear();
	for (int i = 0; i < prop->numRecord; ++i)
	{
		int id = 0;
		int size = record.CollectionSizeRecord(&write[idx]);
		id = record.CollectionID(&write[idx]);

		if (id == -1)
		{
			memmove_s(&write[idx], capacity - idx + 1, &write[idx + size], prop->tableSize - idx);
			deleteRecord++;
			prop->tableSize -= size;
		}
		else
		{
			index.Create(id, idx);
			idx += size;
		}
	}
	prop->numRecord -= deleteRecord;
}

int MugenScript::MugenScriptDatabaseTable::Load(MugenScriptDatabaseFile& file)
{
	if (!file.size())
		return -1;
	auto head = file.distance();
	int numExtent;
	DATABASE_PAGE_SIZE pageSize;
	MugenScriptDatabaseTableProp prop = {};
	file.read(&prop, sizeof(MugenScriptDatabaseTableProp));
	file.seek_begin(head);
	numExtent = prop.numExtent;
	pageSize = prop.pageSize;

	Alloc(numExtent, pageSize);

	for (int i = 0; i < numExtent; ++i)
		for (int j = 0; j < MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE; ++j)
			file.read(&page[pageSize * j + i * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE * pageSize], pageSize);

	column.CollectionMemory(&page[prop.startColumn]);
	index.CollectionMemory(&page[prop.startIndex]);
	record.UpdateColumnData();
	columnIndex.UpdateColumnData();
	columnIndex.CollectionMemory(&page[prop.startColumnIndex]);

	return 0;
}

int MugenScript::MugenScriptDatabaseTable::Save(MugenScriptDatabaseFile& file)
{
	int start = file.distance();
	prop->startIndex = prop->tableSize;
	prop->startColumn = prop->tableSize + index.SizeOfDeployment();
	prop->startColumnIndex = prop->startColumn + column.SizeOfDeployment();

	if (capacity < prop->startColumnIndex+ columnIndex.SizeOfDeployment())
		Realloc(prop->pageSize);

	index.DeployMemory(&page[prop->startIndex], capacity);
	column.DeployMemory(&page[prop->startColumn], capacity);
	columnIndex.DeployMemory(&page[prop->startColumnIndex], capacity);
	//file.write(&prop, sizeof(MugenScriptDatabaseTableProp));

	for (int i = 0; i < prop->numExtent; ++i)
		for (int j = 0; j < MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE; ++j)
			file.write(&page[prop->pageSize * j + i * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE * prop->pageSize], prop->pageSize);

	return file.distance() - start;
}


const int MugenScript::MugenScriptDatabaseTable::NumRecord() const
{
	return prop->numRecord;
}

const int MugenScript::MugenScriptDatabaseTable::NumColumn() const
{
	return column.GetNumColumn();
}

const Mugenshuen::string_t MugenScript::MugenScriptDatabaseTable::NameColumn(int id) const
{
	return column.GetName(id).value.string;
}

const MugenScript::DATABASE_VALUE_TYPE* MugenScript::MugenScriptDatabaseTable::TypesColumn() const
{
	return column.GetTypes();
}

const MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseTable::NamesColumn() const
{
	return column.GetNames();
}

const MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseTable::InitialsColumn() const
{
	return column.GetInitials();
}

const int MugenScript::MugenScriptDatabaseTable::IDFromIndex(int idx) const
{
	int id = 0;
	int pos = 0;
	index.ReadFromIndex(idx, id, pos);
	return id;
}

const int MugenScript::MugenScriptDatabaseTable::FindColumn(Mugenshuen::string_t name) const
{
	return column.Find(name);
}

void MugenScript::MugenScriptDatabaseTable::CopyColumn(MugenScriptDatabaseColumn& o) const
{
	o = column;
}

void MugenScript::MugenScriptDatabaseTable::CopyIndex(MugenScriptDatabaseIndex& o)const
{
	o = index;
}

MugenScript::MugenScriptDatabaseTable::MugenScriptDatabaseTable() :
	page(),
	capacity(),
	column(0, nullptr, nullptr, nullptr),
	record(&column),
	columnIndex(&column)
{
}

MugenScript::MugenScriptDatabaseTable::MugenScriptDatabaseTable(const Mugenshuen::string_t name) :
	page(),
	capacity(),
	column(0, nullptr, nullptr, nullptr),
	record(&column),
	columnIndex(&column)
{
}

MugenScript::MugenScriptDatabaseTable::MugenScriptDatabaseTable(const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials) :
	page(),
	capacity(),
	column(numColumn, names, types, initials),
	record(&column),
	columnIndex(&column)
{
	Construct(name, page, numColumn, names, types, initials);
}

MugenScript::MugenScriptDatabaseTable::~MugenScriptDatabaseTable()
{
	Release();
}


int MugenScript::MugenScriptDatabaseTable::GetNextID() const
{
	return prop->nextID;
}

char* MugenScript::MugenScriptDatabaseTable::GetTableReserved() const
{
	auto tblSize = MugenScriptDatabaseTable::SizeOfDeploymentTable();
	return &page[tblSize];
}

const int MugenScript::MugenScriptDatabaseTable::CapacityReserved() const
{
	return capacity - MugenScriptDatabaseTable::SizeOfDeploymentTable();
}

void MugenScript::MugenScriptDatabaseTable::Construct(const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	column = MugenScriptDatabaseColumn(numColumn, names, types, initials);
	record.UpdateColumnData();
	columnIndex.UpdateColumnData();
	Alloc(1, page);
	strcpy_s(prop->name, sizeof(prop->name), name.C_Str());
	prop->pageSize = page;
	prop->tableSize += sizeof(MugenScriptDatabaseTableProp);

}

void MugenScript::MugenScriptDatabaseTable::Alloc(int numExtent, DATABASE_PAGE_SIZE size)
{
	page = new char[size * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE]();
	capacity = size * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE;
	this->prop = (MugenScriptDatabaseTableProp*)page;
	this->prop->numExtent = numExtent;
}

void MugenScript::MugenScriptDatabaseTable::Realloc(DATABASE_PAGE_SIZE size)
{
	prop->numExtent++;
	char* newPage = new char[size * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE * prop->numExtent]();
	int newCapacity = prop->numExtent * size * MUGEN_SCRIPT_DATABASE_NUM_EXTENT_PAGE;
	if (page)
	{
		memcpy_s(newPage, newCapacity, page, capacity);
		delete[]page;
	}

	page = newPage;
	capacity = newCapacity;
	prop = (MugenScriptDatabaseTableProp*)page;
}

void MugenScript::MugenScriptDatabaseTable::Release()
{
	delete[]page;
	prop->numExtent = 0;
	capacity = 0;
	page = nullptr;
}

bool MugenScript::MugenScriptDatabaseIndex::Create(int id, int pos)
{
	Index newIdx;
	newIdx.id = id;
	newIdx.pos = pos;

	if (idx.find(newIdx))
		return false;

	idx.insert(newIdx);
	return true;
}

void MugenScript::MugenScriptDatabaseIndex::Read(int id, int& pos) const
{
	Index readId;
	int keyIdx = 0;
	readId = id;
	auto n = idx.find(keyIdx, readId);

	if (!n)
	{
		pos = -1;
		return;
	}

	pos = n->keys[keyIdx].pos;
}

void MugenScript::MugenScriptDatabaseIndex::Delete(int id)
{
	int keyIndex = 0;
	Index vIndex;
	vIndex = id;
	auto n = idx.find(vIndex);

	if (!n)
		return;
	idx.remove(vIndex);
}

void MugenScript::MugenScriptDatabaseIndex::Clear()
{
	idx.clear();
}

void MugenScript::MugenScriptDatabaseIndex::ReadFromIndex(int idx, int& id, int& pos)const
{
	if (this->idx.size() < idx)
	{
		id = -1;
		pos = -1;
	}

	auto& n = this->idx[idx];
	id = n.id;
	pos = n.pos;
}

int MugenScript::MugenScriptDatabaseIndex::Size() const
{
	return idx.size();
}

int MugenScript::MugenScriptDatabaseIndex::SizeOfDeployment() const
{
	return sizeof(int) + sizeof(Index) * idx.size();
}

int MugenScript::MugenScriptDatabaseIndex::DeployMemory(char* dst, int capacity) const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue numIdx(idx.size());
	int size = idx.size();
	int writeIndex = 0;
	writeIndex += memio.WriteValue(&dst[writeIndex], capacity - writeIndex, &numIdx);
	for (int i = 0; i < size; ++i)
	{
		auto& v = idx[i];
		MugenScriptDatabaseValue id(v.id);
		MugenScriptDatabaseValue pos(v.pos);
		writeIndex += memio.WriteValue(&dst[writeIndex], capacity - writeIndex, &id);
		writeIndex += memio.WriteValue(&dst[writeIndex], capacity - writeIndex, &pos);
	}

	return writeIndex;
}

int MugenScript::MugenScriptDatabaseIndex::CollectionMemory(const char* src)
{
	Clear();

	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue numIdx(0);
	auto read = src;
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, numIdx, src);

	for (int i = 0; i < numIdx.value.ivalue; ++i)
	{
		MugenScriptDatabaseValue id(0);
		MugenScriptDatabaseValue pos(0);
		Index vIndex;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, id, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, pos, read);

		vIndex.id = id.value.ivalue;
		vIndex.pos = pos.value.ivalue;
		idx.insert(vIndex);
	}

	return read - src;
}

int MugenScript::MugenScriptDatabaseIndex::CollectionFile(MugenScriptDatabaseFile& file)
{
	Clear();

	MugenScriptDatabaseTableDataFileReaderWriter reader;
	MugenScriptDatabaseValue numIdx(0);
	int start = file.distance();
	reader.ReadValue(DATABASE_VALUE_TYPE_INT, numIdx, file);

	for (int i = 0; i < numIdx.value.ivalue; ++i)
	{
		MugenScriptDatabaseValue id(0);
		MugenScriptDatabaseValue pos(0);
		Index vIndex;
		reader.ReadValue(DATABASE_VALUE_TYPE_INT, id, file);
		reader.ReadValue(DATABASE_VALUE_TYPE_INT, pos, file);

		vIndex.id = id.value.ivalue;
		vIndex.pos = pos.value.ivalue;
		idx.insert(vIndex);
	}

	return file.distance() - start;
}

void MugenScript::MugenScriptDatabaseIndex::operator=(const MugenScriptDatabaseIndex& o)
{
	idx = o.idx;
}

void MugenScript::MugenScriptDatabaseIndex::operator=(MugenScriptDatabaseIndex&& o)
{
	idx = std::move(o.idx);
}

MugenScript::MugenScriptDatabaseIndex::MugenScriptDatabaseIndex(const MugenScriptDatabaseIndex& o)
{
	*this = o;
}

MugenScript::MugenScriptDatabaseIndex::MugenScriptDatabaseIndex(MugenScriptDatabaseIndex&& o)
{
	*this = std::move(o);
}

bool MugenScript::MugenScriptDatabaseFile::exist(const Mugenshuen::string_t& fileName)const
{
	Mugenshuen::string_t dir = "resource/";
	Mugenshuen::string_t path = dir + fileName;
	return FileSystem::Exists(path.C_Str());
}

void MugenScript::MugenScriptDatabaseFile::open(const Mugenshuen::string_t& fileName)
{
	Mugenshuen::string_t dir = "resource/";
	Mugenshuen::string_t path = dir + fileName;
	file = FileSystem::fm.open(path.C_Str(), FileSystem::FILE_MODE_BINARY);
	this->fileName = fileName;
}

void MugenScript::MugenScriptDatabaseFile::remove(const Mugenshuen::string_t& fileName) const
{
	Mugenshuen::string_t dir = "resource/";
	Mugenshuen::string_t path = dir + fileName;
	FileSystem::Delete(path.C_Str());
}

void MugenScript::MugenScriptDatabaseFile::close()
{
	FileSystem::fm.close(file);
	fileName = "";
}

void MugenScript::MugenScriptDatabaseFile::read(void* dst, size_type size)const
{
	file.read(dst, size);
}

void MugenScript::MugenScriptDatabaseFile::write(void* src, size_type size)
{
	file.write(src, size);
}

void MugenScript::MugenScriptDatabaseFile::gets(char* dst, size_type buffer)const
{
	file.gets(dst, buffer);
}

void MugenScript::MugenScriptDatabaseFile::begin()const
{
	file.begin();
}

void MugenScript::MugenScriptDatabaseFile::seek_begin(size_type size)const
{
	file.seek_begin(size);
}

void MugenScript::MugenScriptDatabaseFile::seek_current(size_type size)const
{
	file.seek_current(size);
}

int MugenScript::MugenScriptDatabaseFile::distance()const
{
	return file.distance();
}

int MugenScript::MugenScriptDatabaseFile::size()const
{
	return file.size();
}

int MugenScript::MugenScriptDatabaseFile::lens()const
{
	return file.lens();
}

bool MugenScript::MugenScriptDatabaseFile::is_loaded() const
{
	return file.is_loaded();
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseFile::name() const
{
	return fileName;
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseFile::path() const
{
	return file.get_path().string().c_str();
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseFile::stem() const
{
	return FileSystem::Stem(fileName.C_Str()).string().c_str();
}

int MugenScript::MugenScriptDatabaseTableEx::Create(int num, const MugenScriptDatabaseValue* value)
{
	Mugenshuen::array_t<MugenScriptDatabaseValue> buff;
	buff.reserve(NumColumn());
	for (int i = 0; i < buff.size(); ++i)
		buff[i] = value[i];
	int id = GetNextID();
	for (int i = 0; i < features.size(); ++i)
		if(features[i])
			features[i]->Create(id, i, buff.data());
	id = MugenScriptDatabaseTable::Create(num, buff.data());
	return id;
}

void MugenScript::MugenScriptDatabaseTableEx::Delete(int id)
{
	MugenScriptDatabaseTable::Delete(id);
	for (int i = 0; i < features.size(); ++i)
		if(features[i])
			features[i]->Delete(id);
}

int MugenScript::MugenScriptDatabaseTableEx::Read(int id, MugenScriptDatabaseValue* dst) const
{
	int numColumn = NumColumn();
	int result = MugenScriptDatabaseTable::Read(id, dst);
	for (int i = 0; i < features.size(); ++i)
		if(features[i])
			features[i]->Read(id, i, dst);
	return result;
}

void MugenScript::MugenScriptDatabaseTableEx::Replace(int id, const MugenScriptDatabaseValue* value)
{
	Mugenshuen::array_t<MugenScriptDatabaseValue> buff;
	int numColumn = NumColumn();
	buff.reserve(numColumn);
	for (int i = 0; i < numColumn; ++i)
		buff[i] = value[i];

	for (int i = 0; i < features.size(); ++i)
		if(features[i])
			features[i]->Replace(id, i, buff.data());
	MugenScriptDatabaseTable::Replace(id, buff.data());
}

MugenScript::MugenScriptDatabaseTableEx::MugenScriptDatabaseTableEx() :
	MugenScriptDatabaseTable(),
	features(),
	exTableRecord()
{
}

MugenScript::MugenScriptDatabaseTableEx::MugenScriptDatabaseTableEx(const Mugenshuen::string_t database, const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials):
	MugenScriptDatabaseTable(),
	features(),
	exTableRecord()
{
	ConstructFeatures(database,name,numColumn, names, types, initials);
	Construct(name, page, numColumn, names, types, initials);
}

MugenScript::MugenScriptDatabaseTableEx::~MugenScriptDatabaseTableEx()
{
	for (int i = 0; i < features.size(); ++i)
		delete features[i];
}

int MugenScript::MugenScriptDatabaseTableEx::Load(MugenScriptDatabaseFile& file)
{
	int index = 0;
	auto result = MugenScriptDatabaseTable::Load(file);
	Mugenshuen::array_t<Mugenshuen::string_t> names(NumColumn());
	for (int i = 0; i < NumColumn(); ++i)
		names[i] = NamesColumn()[i].value.string;
	ConstructFeatures(file.stem(),Name(), NumColumn(), names.data(), TypesColumn(), InitialsColumn());
	const char* read = GetTableReserved();
	for (int i = 0; i < features.size(); ++i)
		if (features[i])
		{
			features[i]->CollectionMemory(&read[index]);
			index += features[i]->SizeOfDeployment();
		}
	return result;
}

int MugenScript::MugenScriptDatabaseTableEx::Save(MugenScriptDatabaseFile& file)
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	auto write = GetTableReserved();
	int index = 0;
	for (int i = 0; i < features.size(); ++i)
		if (features[i])
			index += features[i]->DeployMemory(&write[index], CapacityReserved() - index);
	
	auto result = MugenScriptDatabaseTable::Save(file);
	return this->SizeOfDeploymentTable();
}

int MugenScript::MugenScriptDatabaseTableEx::SizeOfDeploymentTable() const
{
	return MugenScriptDatabaseTable::SizeOfDeploymentTable() + SizeOfDeploymentFeatures();
}

int MugenScript::MugenScriptDatabaseTableEx::GetConditionRelationIndex(int columnID)
{
	if (TypesColumn()[columnID] != DATABASE_VALUE_TYPE_EX_TRIGGER)
		return -1;
	auto cond = (MugenScriptDatabaseTableTrigger*)features[columnID];
	return cond->GetRelationColumn();
}

bool MugenScript::MugenScriptDatabaseTableEx::CheckCondition(int columnID, int recordID)
{
	if (TypesColumn()[columnID] != DATABASE_VALUE_TYPE_EX_TRIGGER)
		return false;
	if (exTableRecord.size() != NumColumn())
		exTableRecord.resize(NumColumn());
	auto cond = (MugenScriptDatabaseTableTrigger*)features[columnID];
	if (cond)
	{
		Read(recordID, exTableRecord.data());
		return cond->CheckTrigger(recordID);
	}
	return false;
}

const MugenScript::DATABASE_VALUE_TYPE MugenScript::MugenScriptDatabaseTableEx::GetConditionRelatedInitial(int columnID) const
{
	auto cond = (MugenScriptDatabaseTableTrigger*)features[columnID];
	return cond->GetRelationColumnType();
}

int MugenScript::MugenScriptDatabaseTableEx::SizeOfDeploymentFeatures() const
{
	int size = 0;
	for (int i = 0; i < features.size(); ++i)
		if(features[i])
			size += features[i]->SizeOfDeployment();
	return size;
}

void MugenScript::MugenScriptDatabaseTableEx::ConstructFeatures(const Mugenshuen::string_t database, const Mugenshuen::string_t table, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	features.resize(numColumn);
	for (int i = 0; i < numColumn; ++i)
	{
		DATABASE_VALUE_TYPE type = types[i];
		switch (type)
		{
		case MugenScript::DATABASE_VALUE_TYPE_EX_TRIGGER:
			features[i] = new MugenScriptDatabaseTableTrigger;
			break;
		case MugenScript::DATABASE_VALUE_TYPE_EX_EVENT:
			features[i] = new MugenScriptDatabaseTableEvent;
			break;
		case MugenScript::DATABASE_VALUE_TYPE_EX_SCRIPT:
			features[i] = new MugenScriptDatabaseTableScript;
			break;
		case MugenScript::DATABASE_VALUE_TYPE_EX_REL:
			features[i] = new MugenScriptDatabaseTableRelationEx;
			break;

		case MugenScript::DATABASE_VALUE_TYPE_EX_NUM_TYPE:
			break;
		default:
			break;
		}
		if (features[i])
		{
			features[i]->SetDatabaseName(database);
			features[i]->SetTableName(table);
			features[i]->CreateFeatrue(numColumn, i, names, types, initials);
		}
	}
}

void MugenScript::MugenScriptDatabaseTableDataFileReaderWriter::ReadValue(DATABASE_VALUE_TYPE type, MugenScriptDatabaseValue& dst, MugenScriptDatabaseFile& file) const
{
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		file.read(&dst.value.ivalue, sizeof(int));
		dst.type = DATABASE_VALUE_TYPE_INT;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		file.read(&dst.value.dvalue, sizeof(double));
		dst.type = DATABASE_VALUE_TYPE_DBL;
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		int len = file.lens();
		dst.value.string = new char[len + 1];
		file.gets(dst.value.string, len + 1);
		dst.type = DATABASE_VALUE_TYPE_STR;
		break;
	}
	default:
		file.read(&dst.value.ivalue, sizeof(int));
		dst.type = DATABASE_VALUE_TYPE_INVALID;
		break;
	}
}


int MugenScript::MugenScriptDatabaseTableDataFileReaderWriter::WriteValue(MugenScriptDatabaseValue* src, MugenScriptDatabaseFile& file) const
{
	auto type = src->type;
	int size = 0;
	switch (type)
	{
	case MugenScript::DATABASE_VALUE_TYPE_INT:
		file.write(&src->value.ivalue, sizeof(int));
		size = src->size();
		break;
	case MugenScript::DATABASE_VALUE_TYPE_DBL:
		file.write(&src->value.dvalue, sizeof(double));
		size = src->size();
		break;
	case MugenScript::DATABASE_VALUE_TYPE_STR:
	{
		int len = src->size();
		if (src->value.string)
			file.write(src->value.string, len);
		size = len;
		break;
	}
	case MugenScript::DATABASE_VALUE_TYPE_INVALID:
		file.write(&src->value.ivalue, sizeof(int));
		size = src->size();
		break;

	default:
		file.write(&src->value.ivalue, sizeof(int));
		size = src->size();
		break;
	}

	return size;
}

MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseRelation::RelationData::GetRelation(int recordID)
{
	if (recordID == -1)
		return nullptr;
	int pos = 0;
	MugenScriptDatabaseReader reader;
	reader.ReadTableRecord(tableStart, recordID, relationTableIndex, relationTableRecord, file);

	return relationTableRecord.Get();
}

MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseRelation::RelationData::GetRelation(const MugenScriptDatabaseValue& recordID)
{
	int id = 0;
	MugenScriptDatabaseReader reader;
	relationTableColumnIndex.Read(relationColumn, recordID, 1, &id);
	if (id == -1)
		return nullptr;
	reader.ReadTableRecord(tableStart, id, relationTableIndex, relationTableRecord, file);
	return relationTableRecord.Get();
}

MugenScript::MugenScriptDatabaseRelation::RelationData::RelationData(int columnID, const Mugenshuen::string_t relationColumn, int tableStart, const MugenScriptDatabaseTableProp& prop,  MugenScriptDatabaseFile& file) :
	columnID(columnID),
	tableStart(tableStart),
	relationColumn(),
	file(),
	relationTableColumn(0, nullptr, nullptr, nullptr),
	relationTableColumnIndex(&relationTableColumn),
	relationTableIndex(),
	relationTableRecord(&relationTableColumn)
{
	MugenScriptDatabaseReader reader;
	auto start = file.distance();
	reader.ReadTableColumn(tableStart, prop, relationTableColumn, file);
	reader.ReadTableIndex(tableStart, prop, relationTableIndex, file);
	
	relationTableRecord.UpdateColumnData();
	relationTableColumnIndex.UpdateColumnData();
	relationTableColumnIndex.CreateColumnIndex(relationTableColumn.Find(relationColumn), relationTableIndex, tableStart, file);

	this->relationColumn = relationTableColumn.Find(relationColumn);
	if (this->relationColumn == -1)
		return;
	this->file.open(file.name());
}

MugenScript::MugenScriptDatabaseRelation::RelationData::~RelationData()
{
	if(file.is_loaded())
		file.close();
}

int MugenScript::MugenScriptDatabaseRelation::Create(int id, MugenScriptDatabaseValue* value)
{
	//for(int i=0;i<column->GetNumColumn();++i)
	//	if (column->GetType(i) == DATABASE_VALUE_TYPE_REL)
	//	{
	//		TableData data;
	//		data.id = id;
	//		data.value = value[i];
	//		relationTableData[i].insert(data);
	//		if (!relationList[i])
	//		{
	//			value[i].type = DATABASE_VALUE_TYPE_REL;
	//			value[i].value.relation.id = -1;
	//			//relationTableData[i].insert(data);
	//			continue;
	//		}
	//		int id = 0;
	//		relationList[i]->relationTableColumnIndex.Read(i, value[i], 1, &id);
	//		value[i].type = DATABASE_VALUE_TYPE_REL;
	//		value[i].value.relation.id = id;
	//		//relationTableData[i].insert(data);
	//	}
	return 0;
}

void MugenScript::MugenScriptDatabaseRelation::Delete(int id)
{
	for (int i = 0; i < column->GetNumColumn(); ++i)
	{
		TableData data;
		data = id;
		relationTableData[i].remove(data);
	}
}

void MugenScript::MugenScriptDatabaseRelation::Read(int id, MugenScriptDatabaseValue* dst) const
{
	//for (int i = 0; i < column->GetNumColumn(); ++i)
	//	if (column->GetType(i) == DATABASE_VALUE_TYPE_REL)
	//	{
	//		TableData data;
	//		int keyIdx = 0;
	//		data = id;
	//		auto n = relationTableData[i].find(keyIdx, data);
	//		if (!n)
	//		{
	//			dst[i] = MugenScriptDatabaseValue((DATABASE_VALUE_TYPE)DATABASE_VALUE_TYPE_INVALID);
	//			break;
	//		}
	//		dst[i] = n->keys[keyIdx].value;
	//	}
}

void MugenScript::MugenScriptDatabaseRelation::Replace(int id, MugenScriptDatabaseValue* value)
{
	Delete(id);
	Create(id, value);
}

int MugenScript::MugenScriptDatabaseRelation::CreateRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName)
{
	auto path = relDatabaseName + ".mdb";
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseFile file;
	MugenScriptDatabaseTableProp prop;
	MugenScriptDatabaseReader reader;
	file.open(path);
	int tablePos = GetTablePos(relDatabaseName, relTableName,file);
	if (tablePos == -1)
	{
		file.close();
		return - 1;
	}
	reader.ReadTableHandle(prop, tablePos, file);
	RelationData* ret = new RelationData(columnID, relationColumn, tablePos, prop, file);
	if (ret->relationColumn == -1)
	{
		delete ret;
		file.close();
		return -1;
	}
	file.close();
	relationList[columnID] = ret;
	auto& tableVal = relationTableData[columnID];
	//tableVal.clear();
	for (int i = 0; i < tableVal.size(); ++i)
		tableVal[i].value = ret->relationTableColumn.GetInitial(ret->relationColumn);

	ret->databaseName = relDatabaseName;
	ret->tableName = prop.name;
	ret->relationColumnName = relationColumn;
	return ret->relationTableColumn.GetNumColumn();
}

int MugenScript::MugenScriptDatabaseRelation::ReplaceRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName)
{
	auto& relColumn = relationList[columnID]->relationTableColumn;
	if (column->GetType(relationList[columnID]->columnID) != relColumn.GetType(relationList[columnID]->relationColumn))
		return -1;
	DeleteRelation(columnID);

	auto path = relDatabaseName + ".mdb";
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseFile file;
	MugenScriptDatabaseTableProp prop;
	MugenScriptDatabaseReader reader;
	file.open(path);
	int tablePos = GetTablePos(relDatabaseName, relTableName, file);
	if (tablePos == -1)
	{
		file.close();
		return -1;
	}
	reader.ReadTableHandle(prop, tablePos, file);
	RelationData* ret = new RelationData(columnID, relationColumn, tablePos, prop, file);
	if (ret->relationColumn == -1)
	{
		delete ret;
		file.close();
		return -1;
	}
	file.close();
	relationList[columnID] = ret;
	//auto& tableVal = relationTableData[columnID];

	ret->databaseName = relDatabaseName;
	ret->tableName = prop.name;
	ret->relationColumnName = relationColumn;
	return ret->relationTableColumn.GetNumColumn();
}

void MugenScript::MugenScriptDatabaseRelation::DeleteRelation(int columnID)
{
	if (relationList[columnID])
	{
		delete relationList[columnID];
		relationList[columnID] = nullptr;
	}
}

int MugenScript::MugenScriptDatabaseRelation::ReadRelation(int id,int columnID, int dstBuff, MugenScriptDatabaseValue* dst)
{
	if (!relationList[columnID])
		return -1;
	TableData data;
	int keyIdx = 0;
	data = id;
	auto n = relationTableData[columnID].find(keyIdx, data);
	if (!n)
		return -1;
	int numColumn = relationList[columnID]->relationTableColumn.GetNumColumn();
	auto record = relationList[columnID]->GetRelation(n->keys[keyIdx].value);
	if (!record)
		return -1;
	for (int j = 0; j < dstBuff && j < numColumn; ++j)
		dst[j] = record[j];
	return 0;
}

MugenScript::MugenScriptDatabaseRelation::RelationData* MugenScript::MugenScriptDatabaseRelation::Relation(int columnID)
{
	return relationList[columnID];
}

const MugenScript::MugenScriptDatabaseRelation::RelationData* MugenScript::MugenScriptDatabaseRelation::Relation(int columnID) const
{
	return relationList[columnID];
}

void MugenScript::MugenScriptDatabaseRelation::UpdateColumnData()
{
	relationList.resize(column->GetNumColumn());
	relationTableData.resize(column->GetNumColumn());
}

void MugenScript::MugenScriptDatabaseRelation::UpdateReference(const char* tableStart, const MugenScriptDatabaseIndex& idx)
{
	//auto read = tableStart;
	//MugenScriptDatabaseRecord record(column);
	//for (int i = 0; i < idx.Size(); ++i)
	//{
	//	int id = 0, pos = 0;
	//	idx.ReadFromIndex(i,id, pos);
	//	if (id == -1 || pos == -1)
	//		continue;
	//	record.CollectionMemory(&read[pos]);
	//	for (int j = 0; j < column->GetNumColumn(); ++j)
	//	{
	//		if (column->GetType(j) != DATABASE_VALUE_TYPE_REL || !relationList[j])
	//			continue;
	//		TableData data;
	//		data = id;
	//		int keyIdx = 0;
	//		auto n = relationTableData[j].find(keyIdx, data);
	//		int relatedID = record.Get()[j].value.relation.id;
	//		if (!n || relatedID == -1)
	//			continue;
	//		auto relatedRec = relationList[j]->GetRelation(relatedID);
	//		n->keys[keyIdx].value = relatedRec[j];
	//	}
	//}
}

int MugenScript::MugenScriptDatabaseRelation::SizeOfDeployment() const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	int idx = 0;

	for (int i = 0; i < relationList.size(); ++i)
	{
		if (!relationList[i])
		{
			idx += sizeof(int);
			continue;
		}

		MugenScriptDatabaseValue vColumnID;
		MugenScriptDatabaseValue vDatabaseName;
		MugenScriptDatabaseValue vTableName;
		MugenScriptDatabaseValue vColumnRel;
		vColumnID = relationList[i]->columnID;
		vDatabaseName = relationList[i]->databaseName.C_Str();
		vTableName = relationList[i]->tableName.C_Str();
		vColumnRel = relationList[i]->relationColumnName.C_Str();

		idx += vColumnID.size();
		idx += vDatabaseName.size();
		idx += vTableName.size();
		idx += vColumnRel.size();
	}

	for (int i = 0; i < relationTableData.size(); ++i)
	{
		MugenScriptDatabaseValue vSize;
		auto& tableData = relationTableData[i];

		vSize = tableData.size();
		idx += vSize.size();
		for (int j = 0; j < tableData.size(); ++j)
		{
			MugenScriptDatabaseValue vID;
			MugenScriptDatabaseValue vValue;
			
			idx += sizeof(int);
			idx += sizeof(DATABASE_VALUE_TYPE);
			idx += tableData[j].value.size();
		}
	}
	return idx;
}

int MugenScript::MugenScriptDatabaseRelation::DeployMemory(char* dst, int capacity) const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	int idx = 0;

	for (int i = 0; i < relationList.size(); ++i)
	{
		if (!relationList[i])
		{
			MugenScriptDatabaseValue vInvalid;
			vInvalid = -1;
			idx += memio.WriteValue(&dst[idx], capacity - idx, &vInvalid);
			continue;
		}

		MugenScriptDatabaseValue vColumnID;
		MugenScriptDatabaseValue vDatabaseName;
		MugenScriptDatabaseValue vTableName;
		MugenScriptDatabaseValue vColumnRelName;
		MugenScriptDatabaseValue vColumnRelType;
		vColumnID = relationList[i]->columnID;
		vDatabaseName = relationList[i]->databaseName.C_Str();
		vTableName = relationList[i]->tableName.C_Str();
		vColumnRelName = relationList[i]->relationColumnName.C_Str();
		vColumnRelType = (int)relationList[i]->relationTableColumn.GetType(relationList[i]->relationColumn);

		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vDatabaseName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vTableName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnRelName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnRelType);
	}

	for (int i = 0; i < relationTableData.size(); ++i)
	{
		MugenScriptDatabaseValue vSize;
		auto& tableData = relationTableData[i];

		vSize = tableData.size();
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vSize);
		for (int j = 0; j < tableData.size(); ++j)
		{
			MugenScriptDatabaseValue vID;
			MugenScriptDatabaseValue vType;
			MugenScriptDatabaseValue vValue;
			vID = tableData[j].id;
			vValue = tableData[j].value;
			vType = (int)vValue.type;
			idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
			idx += memio.WriteValue(&dst[idx], capacity - idx, &vType);
			idx += memio.WriteValue(&dst[idx], capacity - idx, &vValue);
		}
	}

	return idx;
}

int MugenScript::MugenScriptDatabaseRelation::CollectionMemory(const char* src)
{
	MugenScriptDatabaseReader reader;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	auto read = src;
	for (int i = 0; i < relationList.size(); ++i)
	{
		MugenScriptDatabaseValue vColumnID;
		MugenScriptDatabaseValue vDatabaseName;
		MugenScriptDatabaseValue vTableName;
		MugenScriptDatabaseValue vColumnRel;
		MugenScriptDatabaseValue vColumnRelType;
		MugenScriptDatabaseFile file;
		MugenScriptDatabaseTableProp tableProp;
		MugenScriptDatabaseColumn relColumn(0, 0, 0, 0);
		Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> map;
		MugenScriptDatabaseTableMapData data;
		int keyIdx = 0;

		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnID, read);
		if (vColumnID.value.ivalue == -1)
			continue;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vDatabaseName, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vTableName, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vColumnRel, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnRelType, read);

		Mugenshuen::string_t path = vDatabaseName.value.string;
		path += ".mdb";
		if (!file.exist(path))
			continue;
		file.open(path);
		if (!file.is_loaded())
			continue;
		reader.ReadDatabaseMap(map, file);
		data.name = vTableName.value.string;
		auto n = map.find(keyIdx, data);
		if (!n)
			continue;
		reader.ReadTableHandle(tableProp, n->keys[keyIdx].pos, file);
		reader.ReadTableColumn(n->keys[keyIdx].pos, tableProp, relColumn, file);
		int relColumnID = relColumn.Find(vColumnRel.value.string);
		if (relColumnID == -1 || relColumn.GetType(relColumnID) != vColumnRelType.value.ivalue)
			continue;
		relationList[i] = new RelationData(i, vColumnRel.value.string, n->keys[keyIdx].pos, tableProp, file);
		relationList[i]->databaseName = vDatabaseName.value.string;
		relationList[i]->tableName = vTableName.value.string;
		relationList[i]->relationColumnName = vColumnRel.value.string;
	}
	for (int i = 0; i < relationTableData.size(); ++i)
	{
		MugenScriptDatabaseValue vSize;
		auto& tableData = relationTableData[i];

		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, read);
		if (vSize.value.ivalue == 0)
			continue;

		for (int j = 0;j < vSize.value.ivalue; ++j)
		{
			TableData data;
			MugenScriptDatabaseValue vID;
			MugenScriptDatabaseValue vType;
			MugenScriptDatabaseValue vValue;
			read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);
			read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vType, read);
			read = memio.ReadValue((DATABASE_VALUE_TYPE)vType.value.ivalue, vValue, read);
			data.id = vID.value.ivalue;
			data.value = vValue;
			tableData.insert(data);
		}
	}

	return 0;
}


MugenScript::MugenScriptDatabaseRelation::~MugenScriptDatabaseRelation()
{
	for (int i = 0; i < relationList.size(); ++i)
		delete relationList[i];
}

int MugenScript::MugenScriptDatabaseRelation::GetTablePos(const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName,const MugenScriptDatabaseFile& file) const
{
	Mugenshuen::string_t path;
	MugenScriptDatabaseReader reader;
	int keyIdx = 0;
	MugenScriptDatabaseTableMapData mapData;
	Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> map;
	path = dbName + ".mdb";
	if (!file.is_loaded())
		return -1;
	reader.ReadDatabaseMap(map,file);
	mapData.name = talbeName;
	auto n = map.find(keyIdx,mapData);
	if (!n)
		return -1;
	return n->keys[keyIdx].pos;
}

int MugenScript::MugenScriptDatabaseTableScript::Create(const int id, const int column, MugenScriptDatabaseValue* value)
{
	if (!value)
		return -1;
	int pos = 0;
	index.Read(id, pos);
	if (pos != -1)
		return -1;
	assert(value[column].type == DATABASE_VALUE_TYPE_STR);
	int len = value[column].size();
	index.Create(id, scriptSize);
	file.seek_begin(scriptSize);
	file.write(&len, sizeof(len));
	file.write(value[column].value.string, len + 1);
	scriptSize += len + sizeof(int) + 1;
	return id;
}

int MugenScript::MugenScriptDatabaseTableScript::Delete(const int id)
{
	index.Delete(id);
	return 0;
}

void MugenScript::MugenScriptDatabaseTableScript::Read(const int id,const int column, MugenScriptDatabaseValue* dst) const
{
	int pos = 0;
	int len = 0;
	char* script = nullptr;
	index.Read(id, pos);
	if (pos == -1)
		return;

	dst[column] = MugenScriptDatabaseValue(0);
	file.seek_begin(pos);
	file.read(&len, sizeof(int));
	script = new char[len + 1];
	file.read(script, len + 1);
	dst[column].value.string = script;
	dst[column].type = DATABASE_VALUE_TYPE_STR;
}

void MugenScript::MugenScriptDatabaseTableScript::Replace(const int id,const int column, MugenScriptDatabaseValue* value)
{
	int pos = 0;
	int len = 0;
	index.Read(id, pos);
	if (pos == -1)
		return;
	MugenScriptDatabaseValue cur(0);
	Read(id, 0, &cur);

	if (value[column].size() < cur.size())
	{
		len = strlen(value[column].value.string);
		file.seek_begin(pos);
		file.write(&len, sizeof(len));
		file.write(value[column].value.string, len + 1);
	}
	else
	{
		index.Delete(id);
		Create(id, column, value);
	}
}

void MugenScript::MugenScriptDatabaseTableScript::Update()
{
	struct ScriptMap
	{
		int id;
		int pos;
		int size;
		operator int()const { return pos; };
		void operator=(int v) { pos = v; }
	};
	Mugenshuen::btree_t<ScriptMap> scriptMap;
	for (int i = 0; i < index.Size(); ++i)
	{
		ScriptMap map;

		index.ReadFromIndex(i, map.id, map.pos);
		file.seek_begin(map.pos);
		file.read(&map.size, sizeof(map.size));
		scriptMap.insert(map);
	}

	scriptSize = 0;
	for (int i = 0; i < scriptMap.size(); ++i)
	{
		auto& n = scriptMap[i];
		if (n.pos == scriptSize)
		{
			scriptSize += n.size;
			continue;
		}

		auto id = n.id;
		MugenScriptDatabaseValue script;
		Read(id,0, &script);
		Delete(id);
		Create(id, -1, &script);
	}
}

int MugenScript::MugenScriptDatabaseTableScript::SizeOfDeployment() const
{
	return sizeof(int) + index.SizeOfDeployment();
}

int MugenScript::MugenScriptDatabaseTableScript::DeployMemory(char* dst, int capacity) const
{
	int idx = sizeof(int);
	*(int*)dst = scriptSize;
	idx += index.DeployMemory(&dst[idx], capacity - idx);
	return idx;
}

int MugenScript::MugenScriptDatabaseTableScript::CollectionMemory(const char* src)
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	if (file.is_loaded())
		file.close();
	file.open(path);
	scriptSize = *(int*)src;
	return index.CollectionMemory(&src[sizeof(int)]);
}

void MugenScript::MugenScriptDatabaseTableScript::CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	path = database + "#" + table;
	file.open(GetPathScript(path));
}

MugenScript::MugenScriptDatabaseTableScript::~MugenScriptDatabaseTableScript()
{
	if (file.is_loaded())
		file.close();
}


Mugenshuen::string_t MugenScript::MugenScriptDatabaseTableScript::GetPathScript(const Mugenshuen::string_t& fileName)
{
	Mugenshuen::string_t dir = "script/";
	Mugenshuen::string_t ext = ".mst";
	return dir + fileName + ext;
}

void MugenScript::IMugenScriptDatabaseFeature::SetTableName(const Mugenshuen::string_t& name)
{
	table = name;
}

void MugenScript::IMugenScriptDatabaseFeature::SetDatabaseName(const Mugenshuen::string_t& name)
{
	database = name;
}

int MugenScript::MugenScriptDatabaseTableTrigger::Create(const int id, const int column, MugenScriptDatabaseValue* value)
{
	MugenScriptDatabaseTableRelationEx::Create(id, column, value);
	MugenScriptConditionData data;
	data = id;
	data.command = value[column].userData;
	auto n = index.find(data);
	if (n)
		return -1;

	index.insert(data);

	return id;
}

int MugenScript::MugenScriptDatabaseTableTrigger::Delete(const int id)
{
	MugenScriptDatabaseTableRelationEx::Delete(id);
	MugenScriptConditionData cond;
	cond.id = id;
	index.remove(cond);
	return 0;
}

void MugenScript::MugenScriptDatabaseTableTrigger::Read(const int id,const int column, MugenScriptDatabaseValue* dst) const
{
	MugenScriptDatabaseTableRelationEx::Read(id, column, dst);
}

void MugenScript::MugenScriptDatabaseTableTrigger::Replace(const int id, const int column, MugenScriptDatabaseValue* value)
{
	Delete(id);
	Create(id, column, value);
}

void MugenScript::MugenScriptDatabaseTableTrigger::Update()
{
}

int MugenScript::MugenScriptDatabaseTableTrigger::SizeOfDeployment() const
{
	int idx = 0;
	idx += sizeof(int) * 2;
	idx += sizeof(int) * 2 * index.size();
	idx += MugenScriptDatabaseTableRelationEx::SizeOfDeployment();
	
	return idx;
}

int MugenScript::MugenScriptDatabaseTableTrigger::DeployMemory(char* dst, int capacity) const
{

	int idx = 0;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue relationID(relationColumnID);
	MugenScriptDatabaseValue numIndex(index.size());

	idx += memio.WriteValue(&dst[idx], capacity - idx, &relationID);
	idx += memio.WriteValue(&dst[idx], capacity - idx, &numIndex);


	for (int i = 0; i < numIndex.value.ivalue; ++i)
	{
		auto& n = index[i];
		MugenScriptDatabaseValue vID(n.id);
		MugenScriptDatabaseValue vCommand(n.command);
		vCommand.value.ivalue &= ~(MugenScriptTriggerBehavior_TriggerFlag);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vCommand);
	}
	idx += MugenScriptDatabaseTableRelationEx::DeployMemory(&dst[idx], capacity - idx);
	return idx;
}

int MugenScript::MugenScriptDatabaseTableTrigger::CollectionMemory(const char* src)
{
	auto read = src;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue relationID;
	MugenScriptDatabaseValue numIndex;
	MugenScriptDatabaseValue relationColumnType;
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, relationID, read);
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, numIndex, read);
	for (int i = 0; i < numIndex.value.ivalue; ++i)
	{
		MugenScriptConditionData cond;
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vCommand;

		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vCommand, read);

		cond.id = vID.value.ivalue;
		cond.command = vCommand.value.ivalue;

		index.insert(cond);
	}
	relationType = (DATABASE_VALUE_TYPE)relationColumnType.value.ivalue;
	relationColumnID = relationID.value.ivalue;
	MugenScriptDatabaseTableRelationEx::CollectionMemory(read);
	return (read - src) + MugenScriptDatabaseTableRelationEx::SizeOfDeployment();
}

void MugenScript::MugenScriptDatabaseTableTrigger::CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	this->columnID = columnID;
	this->types.resize(numColumn);
	this->names.resize(numColumn);
	for (int i = 0; i < numColumn; ++i)
	{
		this->types[i] = types[i];
		this->names[i] = name[i];
	}
	relationColumnID = -1;
}


bool MugenScript::MugenScriptDatabaseTableTrigger::CheckTrigger(const int id)
{
	MugenScriptConditionData data;
	int keyIndex = 0;
	bool result = false;
	data = id;
	auto n = index.find(keyIndex, data);
	if (!n)
		return false;
	ConditionCommand cmd = n->keys[keyIndex].command;
	auto& value = Relation()->GetRelation(id)[columnID]; 
	
	if (MugenScriptTriggerBehavior_CondEdge & n->keys[keyIndex].command && n->keys[keyIndex].prev != value)
	{
		n->keys[keyIndex].prev = value;
		result = true;
	}
	else if (MugenScriptTriggerBehavior_CondEdgeUp & n->keys[keyIndex].command && n->keys[keyIndex].prev < value)
	{
		n->keys[keyIndex].prev = value;
		result = true;
	}
	else if (MugenScriptTriggerBehavior_CondEdgeDown & n->keys[keyIndex].command && n->keys[keyIndex].prev > value)
	{
		n->keys[keyIndex].prev = value;
		result = true;
	}


	if (!(MugenScriptTriggerBehavior_TriggerFlag & n->keys[keyIndex].command) &&(MugenScriptTriggerBehavior_ActOne & n->keys[keyIndex].command))
	{
		result = false;
	}

	if (result && !(MugenScriptTriggerBehavior_TriggerFlag & n->keys[keyIndex].command))
		n->keys[keyIndex].command |= MugenScriptTriggerBehavior_TriggerFlag;
	if (result)
		n->keys[keyIndex].command |= MugenScriptTriggerBehavior_IgEvent;
	else
		n->keys[keyIndex].command &= ~(MugenScriptTriggerBehavior_IgEvent);
	return result;
}

int MugenScript::MugenScriptDatabaseTableTrigger::GetRelationColumn() const
{
	return relationColumnID;
}

MugenScript::DATABASE_VALUE_TYPE MugenScript::MugenScriptDatabaseTableTrigger::GetRelationColumnType() const
{
	if (relationColumnID == -1)
		return DATABASE_VALUE_TYPE_INVALID;
	return relationType;
}

int MugenScript::MugenScriptDatabaseTableEvent::Create(const int id, const int column, MugenScriptDatabaseValue* value)
{
	MugenScriptDatabaseEvent data;
	data = id;
	auto n = eventList.find(data);
	if (!n)
		return -1;
	data.eventName = value[column].value.string;
	data.flags = value[column].userData;
	eventList.insert(data);
	return 0;
}

int MugenScript::MugenScriptDatabaseTableEvent::Delete(const int id)
{
	MugenScriptDatabaseEvent data;
	data = id;
	eventList.remove(data);
	return 0;
}

void MugenScript::MugenScriptDatabaseTableEvent::Read(const int id,const int column, MugenScriptDatabaseValue* dst) const
{
	MugenScriptDatabaseEvent data;
	int keyIdx = 0;
	data = id;
	auto n = eventList.find(keyIdx,data);
	if (!n)
		return;
	dst[column] = n->keys[keyIdx].eventName.C_Str();
	dst[column].userData = n->keys[keyIdx].flags;
}

void MugenScript::MugenScriptDatabaseTableEvent::Replace(const int id, const int column, MugenScriptDatabaseValue* value)
{
	Delete(id);
	Create(id, column, value);
}

void MugenScript::MugenScriptDatabaseTableEvent::Update()
{
}

int MugenScript::MugenScriptDatabaseTableEvent::SizeOfDeployment() const
{
	int size = 0;
	for (int i = 0; i < eventList.size(); ++i)
	{
		MugenScriptDatabaseValue vName = eventList[i].eventName.C_Str();
		size += vName.size();
	}
	return size + (sizeof(int) * eventList.size() * 2) + sizeof(int);
}

int MugenScript::MugenScriptDatabaseTableEvent::DeployMemory(char* dst, int capacity) const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	int idx = 0;
	MugenScriptDatabaseValue vNumEvent(eventList.size());
	idx += memio.WriteValue(&dst[idx], capacity - idx, &vNumEvent);
	for (int i = 0; i < eventList.size(); ++i)
	{
		auto& data = eventList[i];
		MugenScriptDatabaseValue vID(data.id);
		MugenScriptDatabaseValue vFlags(data.flags);
		MugenScriptDatabaseValue vName(data.eventName.C_Str());
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vFlags);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vName);

	}
	return idx;
}

int MugenScript::MugenScriptDatabaseTableEvent::CollectionMemory(const char* src)
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	auto read = src;
	MugenScriptDatabaseValue vNum;
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vNum, read);
	for (int i = 0; i < vNum.value.ivalue; ++i)
	{
		MugenScriptDatabaseEvent data;
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vFlags;
		MugenScriptDatabaseValue vName;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID,read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vFlags, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vName, read);
		data = vID.value.ivalue;
		data.flags = vFlags.value.ivalue;
		data.eventName = vName.value.string;
		eventList.insert(data);
	}
	return read - src;
}

void MugenScript::MugenScriptDatabaseTableEvent::CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
}

bool MugenScript::MugenScriptDatabaseTableEvent::CheckCondition(int id,int numColumn, const MugenScriptDatabaseValue* value)
{
	int keyIndex = 0;
	MugenScriptDatabaseEvent data;
	data = id;
	auto n = eventList.find(keyIndex, data);
	if (!n)
		return false;
	return n->keys[keyIndex].CheckEventCondition(numColumn, value);
}

Mugenshuen::string_t MugenScript::MugenScriptDatabaseTableEvent::GetEventName(int id)
{
	int keyIndex = 0;
	MugenScriptDatabaseEvent data;
	data = id;
	auto n = eventList.find(keyIndex, data);

	if (!n)
		return "";
	return n->keys[keyIndex].eventName;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator<=(const ColumnIndexData& o) const
{
	return value <= o.value;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator>=(const ColumnIndexData& o) const
{
	return value >= o.value;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator<(const ColumnIndexData& o) const
{
	return value < o.value;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator>(const ColumnIndexData& o) const
{
	return value > o.value;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator==(const ColumnIndexData& o) const
{
	return value == o.value && id == o.id;
}

bool MugenScript::MugenScriptDatabaseColumnIndex::ColumnIndexData::operator!=(const ColumnIndexData& o) const
{
	return value != o.value && id != o.id;
}

void MugenScript::MugenScriptDatabaseColumnIndex::CreateColumnIndex(int columnID, const MugenScriptDatabaseIndex& idx, const char* tableHead)
{
	if (columnID == -1 || columnIndexData[columnID].columnID != -1)
		return;
	auto read = tableHead;
	ColumnIndex indexData;
	MugenScriptDatabaseRecord record(column);
	columnIndexData[columnID].columnID = columnID;
	for (int i = 0;i < idx.Size(); ++i)
	{
		ColumnIndexData data;
		int id = 0;
		int pos = 0;
		idx.ReadFromIndex(i, id, pos);
		int size = record.CollectionMemory(&read[pos]);
		data.value = record.Get()[columnID];
		data.id = id;
		columnIndexData[columnID].indexData.insert(data);
	}
}

void MugenScript::MugenScriptDatabaseColumnIndex::CreateColumnIndex(int columnID, const MugenScriptDatabaseIndex& idx,int tableStart, MugenScriptDatabaseFile& file)
{
	if (columnID == -1 || columnIndexData[columnID].columnID != -1)
		return;
	ColumnIndex indexData;
	MugenScriptDatabaseReader reader;
	MugenScriptDatabaseRecord record(column);
	columnIndexData[columnID].columnID = columnID;
	for (int i = 0; i < idx.Size(); ++i)
	{
		ColumnIndexData data;
		int id = 0;
		int pos = 0;
		idx.ReadFromIndex(i, id, pos);
		reader.SeekTableStart(tableStart + pos, file);
		int size = record.CollectionFile(file);
		data.value = record.Get()[columnID];
		data.id = id;
		columnIndexData[columnID].indexData.insert(data);
	}

}

bool MugenScript::MugenScriptDatabaseColumnIndex::Create(int id, const MugenScriptDatabaseValue* value)
{
	ColumnIndex data;
	for (int i = 0; i < column->GetNumColumn(); ++i)
	{
		if (columnIndexData[i].columnID == -1)
			continue;
		ColumnIndexData data;
		data.id = id;
		data.value = value[i];
		columnIndexData[i].indexData.insert(data);
	}
	
	return true;
}

void MugenScript::MugenScriptDatabaseColumnIndex::Read(int columnID, const MugenScriptDatabaseValue& val, int dstBuf, int id[]) const
{
	for (int i = 0; i < dstBuf; ++i)
		id[i] = -1;
	if (columnID == -1 || column->GetNumColumn() <= columnID)
	{
		return;
	}
	ColumnIndexData data;
	data.value = val;
	Mugenshuen::vector_t<ColumnIndexData> dst;
	dst.resize(Count(columnID, val));
	columnIndexData[columnID].indexData.traverse(data, data, dst.size(), dst.data());

	for (int i = 0; i < dst.size() && i < dstBuf; ++i)
		id[i] = dst[i].id;
}

void MugenScript::MugenScriptDatabaseColumnIndex::Delete(int id, const MugenScriptDatabaseValue* val)
{
	for (int i = 0; i < column->GetNumColumn(); ++i)
	{
		if (columnIndexData[i].columnID == -1)
			continue;
		ColumnIndexData data;
		data.value = val[i];
		data.id = id;
		columnIndexData[i].indexData.remove(data);
	}
}

void MugenScript::MugenScriptDatabaseColumnIndex::Clear()
{
	columnIndexData.clear();
}

void MugenScript::MugenScriptDatabaseColumnIndex::ClearColumnIndex(int columnID)
{
	if (columnID == -1 || column->GetNumColumn() <= columnID)
		return;

	columnIndexData[columnID].columnID = -1;
	columnIndexData[columnID].indexData.clear();
}

void MugenScript::MugenScriptDatabaseColumnIndex::Replace(int id, const MugenScriptDatabaseValue* val)
{
	for (int i = 0; i < column->GetNumColumn(); ++i)
	{
		if (columnIndexData[i].columnID == -1)
			continue;

		ColumnIndexData data;
		data.value = val[i];
		data.id = id;
		int keyIndex = 0;
		auto n = columnIndexData[i].indexData.find(data);
		if (!n)
			continue;

		columnIndexData[i].indexData.remove(data);
		columnIndexData[i].indexData.insert(data);
	}
}

void MugenScript::MugenScriptDatabaseColumnIndex::UpdateColumnData()
{
	columnIndexData.resize(column->GetNumColumn());
	for (int i = 0; i < column->GetNumColumn(); ++i)
		columnIndexData[i].columnID = -1;
}

int MugenScript::MugenScriptDatabaseColumnIndex::Count(int columnID, const MugenScriptDatabaseValue& val) const
{
	if (columnID == -1 || column->GetNumColumn() <= columnID)
		return -1;
	ColumnIndexData data;
	data.value = val;

	return columnIndexData[columnID].indexData.count_equal(data,data);
}

int MugenScript::MugenScriptDatabaseColumnIndex::SizeOfDeployment() const
{
	int size = 0;
	for (int i = 0; i < columnIndexData.size(); ++i)
	{
		size += sizeof(int) * 2;
		if (columnIndexData[i].columnID == -1)
			continue;

		for (int j = 0; j < columnIndexData[i].indexData.size(); ++i)
		{
			size += sizeof(int);
			size += columnIndexData[i].indexData[j].value.size();
		}
	}
	return size;
}

int MugenScript::MugenScriptDatabaseColumnIndex::DeployMemory(char* dst, int capacity) const
{
	int idx = 0;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	for (int i = 0; i < columnIndexData.size(); ++i)
	{
		MugenScriptDatabaseValue vColumnID(columnIndexData[i].columnID);
		MugenScriptDatabaseValue vNumIndex(columnIndexData[i].indexData.size());
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vNumIndex);
		
		for (int j = 0; j < columnIndexData[i].indexData.size(); ++j)
		{
			auto& n = columnIndexData[i].indexData[j];
			MugenScriptDatabaseValue vID(n.id);
			idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
			idx += memio.WriteValue(&dst[idx], capacity - idx, &columnIndexData[i].indexData[j].value);
		}
	}
	return 0;
}

int MugenScript::MugenScriptDatabaseColumnIndex::CollectionMemory(const char* src)
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	auto read = src;
	for (int i = 0; i < columnIndexData.size(); ++i)
	{
		MugenScriptDatabaseValue vColumnID(0);
		MugenScriptDatabaseValue vNumIndex(0);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnID, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vNumIndex, read);

		for (int j = 0; j < vNumIndex.value.ivalue; ++j)
		{
			MugenScriptDatabaseValue vID(0);
			MugenScriptDatabaseValue vValue(0);
			ColumnIndexData data;
			read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);
			read = memio.ReadValue(column->GetType(i), vValue, read);
			data.id = vID.value.ivalue;
			data.value = vValue;
			columnIndexData[i].indexData.insert(data);
		}
		columnIndexData[i].columnID = vColumnID.value.ivalue;
	}

	return src - read;
}

int MugenScript::MugenScriptDatabaseColumnIndex::CollectionFile(MugenScriptDatabaseFile& file)
{
	MugenScriptDatabaseTableDataFileReaderWriter fileio;
	int start = file.distance();
	for (int i = 0; i < columnIndexData.size(); ++i)
	{
		MugenScriptDatabaseValue vColumnID(0);
		MugenScriptDatabaseValue vNumIndex(0);
		fileio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnID, file);
		fileio.ReadValue(DATABASE_VALUE_TYPE_INT, vNumIndex, file);

		for (int j = 0; j < vNumIndex.value.ivalue; ++i)
		{
			auto& n = columnIndexData[i].indexData[j];
			MugenScriptDatabaseValue vID(0);
			MugenScriptDatabaseValue vValue(0);
			ColumnIndexData data;
			fileio.ReadValue(column->GetType(i), vID, file);
			fileio.ReadValue(column->GetType(i), vValue, file);
			data.id = vID.value.ivalue;
			data.value = vValue;
			columnIndexData[i].indexData.insert(data);
		}
		columnIndexData[i].columnID = i;
	}
	return file.distance() - start;
}

MugenScript::MugenScriptDatabaseColumnIndex::MugenScriptDatabaseColumnIndex(const MugenScriptDatabaseColumn* column) :
	column(column),
	columnIndexData()
{
	columnIndexData.resize(column->GetNumColumn());
	for (int i = 0; i < column->GetNumColumn(); ++i)
		columnIndexData[i].columnID = -1;
}

void MugenScript::MugenScriptDatabaseReader::ReadDatabaseHandle(MugenScriptDatabaseHandle& dst, const MugenScriptDatabaseFile& file) const
{
	file.seek_begin(0);
	file.read(&dst, sizeof(dst));
}

void MugenScript::MugenScriptDatabaseReader::ReadDatabaseMap(Mugenshuen::btree_t<MugenScriptDatabaseTableMapData>& dst, const MugenScriptDatabaseFile& file) const
{
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseTableMapData map = {};
	ReadDatabaseHandle(dbHandle, file);

	SeekTableStart(dbHandle.mapStart, file);
	int numMap = dbHandle.numTable;

	for (int i = 0; i < numMap; ++i)
	{
		int len = 0;
		file.read(&len, sizeof(len));
		map.name.reserve(len + 1);
		file.read(map.name.Data(), len + 1);
		file.read(&map.pos, sizeof(map.pos));
		dst.insert(map);
	}
}

void MugenScript::MugenScriptDatabaseReader::ReadTableHandle(MugenScriptDatabaseTableProp& dst, int pos, const MugenScriptDatabaseFile& file) const
{
	SeekTableStart(pos, file);
	file.read(&dst, sizeof(dst));
}

void MugenScript::MugenScriptDatabaseReader::ReadTableColumn(const int pos, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseColumn& dst, MugenScriptDatabaseFile& file)const
{
	SeekTableStart(prop.startColumn + pos,file);
	dst.CollectionFile(file);
}

void MugenScript::MugenScriptDatabaseReader::ReadTableIndex(const int pos, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseIndex& dst, MugenScriptDatabaseFile& file) const
{
	SeekTableStart(prop.startIndex+ pos, file);
	dst.CollectionFile(file);
}

void MugenScript::MugenScriptDatabaseReader::ReadTableRecord(const int pos, const int id, const MugenScriptDatabaseIndex& index, const MugenScriptDatabaseColumn& column, MugenScriptDatabaseValue* dst, MugenScriptDatabaseFile& file)
{
	int posRecord = 0;
	MugenScriptDatabaseRecord record(&column);
	index.Read(id, posRecord);
	SeekTableStart(pos + posRecord, file);
	record.CollectionFile(file);
	for (int i = 0; i < column.GetNumColumn(); ++i)
		dst[i] = record.Get()[i];
}

void MugenScript::MugenScriptDatabaseReader::ReadTableRecord(const int pos,const int id, const MugenScriptDatabaseIndex& index, MugenScriptDatabaseRecord& dst, MugenScriptDatabaseFile& file)
{
	int posRecord = 0;
	index.Read(id, posRecord);
	SeekTableStart(pos + posRecord,file);
	dst.CollectionFile(file);
}

void MugenScript::MugenScriptDatabaseReader::SeekTableStart(int size, const MugenScriptDatabaseFile& file) const
{
	file.seek_begin(sizeof(MugenScriptDatabaseHandle) + size);
}

void MugenScript::MugenScriptDatabaseReader::ReadTablePos(int& pos,const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName, const MugenScriptDatabaseFile& file) const
{
	Mugenshuen::string_t path;
	MugenScriptDatabaseReader reader;
	int keyIdx = 0;
	MugenScriptDatabaseTableMapData mapData;
	Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> map;
	path = dbName + ".mdb";
	if (!file.is_loaded())
	{
		pos = -1;
		return;
	}
	reader.ReadDatabaseMap(map, file);
	mapData.name = talbeName;
	auto n = map.find(keyIdx, mapData);
	if (!n)
	{
		pos = -1;
		return;
	}
	pos = n->keys[keyIdx].pos;
}

void MugenScript::MugenScriptDatabaseWriter::WriteDatabaseHandle(const MugenScriptDatabaseHandle src, MugenScriptDatabaseFile& file) const
{
	assert(src.pageSize != 0);
	file.seek_begin(0);
	file.write((void*)&src, sizeof(src));
}

void MugenScript::MugenScriptDatabaseWriter::WriteDatabaseMap(const MugenScriptDatabaseHandle handle, const Mugenshuen::btree_t<MugenScriptDatabaseTableMapData>& src, MugenScriptDatabaseFile& file) const
{
	MugenScriptDatabaseReader reader;
	SeekTableStart(handle.mapStart, file);
	for (int i = 0; i < src.size(); ++i)
	{
		auto data = src[i];
		int len = data.name.Lengh();
		file.write(&len, sizeof(len));
		file.write((void*)data.name.C_Str(), len + 1);
		file.write(&data.pos, sizeof(data.pos));
	}
}

void MugenScript::MugenScriptDatabaseWriter::WriteTableHandle(const MugenScriptDatabaseTableProp& src, const int pos, MugenScriptDatabaseFile& file)
{
	SeekTableStart(pos, file);
	file.write((void*)&src, sizeof(src));
}

void MugenScript::MugenScriptDatabaseWriter::WriteRecord(const int pos, const int id, const MugenScriptDatabaseIndex& index, const MugenScriptDatabaseRecord& src, MugenScriptDatabaseFile& file)
{
	int posRecord = 0;
	index.Read(id, posRecord);
	if (posRecord == -1)
		return;
	Mugenshuen::vector_t<char> dst;
	dst.resize(src.SizeOfDeployment());
	SeekTableStart(pos + posRecord, file);
	
	src.DeployMemory(dst.data(),dst.size());
	file.write(dst.data(), dst.size());
}


void MugenScript::MugenScriptDatabaseWriter::SeekTableStart(int size, const MugenScriptDatabaseFile& file) const
{
	file.seek_begin(sizeof(MugenScriptDatabaseHandle) + size);
}

bool MugenScript::MugenScriptDatabaseEvent::CheckEventCondition(int numValue, const MugenScriptDatabaseValue* value) const
{
	bool result = false;
	bool conditionAnd = flags & MugenScriptEventConditionFlags_And;
	for (int i = 0; i < MugenScriptEventConditionFlags_Num; ++i)
	{
		int mask = 1 << i;
		if (mask & flags)
			continue;
		if (conditionAnd)
		{
			if (!(value[i].userData & MugenScriptTriggerBehavior_IgEvent))
			{
				result = false;
				break;
			}
			else
				result = true;
		}
		else
		{
			if ((value[i].userData & MugenScriptTriggerBehavior_IgEvent))
			{
				result = true;
				break;
			}
			else
				result = false;
		}
	}
	return result;
}

int MugenScript::MugenScriptDatabaseTableRelationEx::Create(int id, int column, MugenScriptDatabaseValue* value)
{
	TableData data;
	data.id = id;
	data.value = value[column];
	relationTableData.insert(data);

	if (relation)
	{
		relation->relationTableColumnIndex.Read(column, value[column], 1, &id);
		value[column].type = DATABASE_VALUE_TYPE_INT;
		value[column].value.ivalue = id;
	}
	else
	{
		value[column].type = DATABASE_VALUE_TYPE_INVALID;
		value[column].value.ivalue = -1;
	}

	return 0;
}

int MugenScript::MugenScriptDatabaseTableRelationEx::Delete(int id)
{
	TableData data;
	data = id;
	relationTableData.remove(data);
	return 0;
}

void MugenScript::MugenScriptDatabaseTableRelationEx::Read(int id, int column, MugenScriptDatabaseValue* dst) const
{
	TableData data;
	int keyIdx = 0;
	data = id;
	auto n = relationTableData.find(keyIdx, data);
	if (!n)
	{
		dst[column] = MugenScriptDatabaseValue((DATABASE_VALUE_TYPE)DATABASE_VALUE_TYPE_INVALID);
		return;
	}
	
	dst[column] = n->keys[keyIdx].value;
}

void MugenScript::MugenScriptDatabaseTableRelationEx::Replace(int id, int column, MugenScriptDatabaseValue* value)
{
	Delete(id);
	Create(id, column,value);
}

void MugenScript::MugenScriptDatabaseTableRelationEx::Update()
{
}

int MugenScript::MugenScriptDatabaseTableRelationEx::CreateRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName)
{
	auto path = relDatabaseName + ".mdb";
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseFile file;
	MugenScriptDatabaseTableProp prop;
	MugenScriptDatabaseReader reader;
	file.open(path);
	int tablePos = GetTablePos(relDatabaseName, relTableName, file);
	if (tablePos == -1)
	{
		file.close();
		return -1;
	}
	reader.ReadTableHandle(prop, tablePos, file);
	RelationData* ret = new RelationData(columnID, relationColumn, tablePos, prop, file);
	if (ret->relationColumn == -1)
	{
		delete ret;
		file.close();
		return -1;
	}
	file.close();
	relation = ret;
	auto& tableVal = relationTableData;
	//tableVal.clear();
	for (int i = 0; i < tableVal.size(); ++i)
		tableVal[i].value = ret->relationTableColumn.GetInitial(ret->relationColumn);

	ret->databaseName = relDatabaseName;
	ret->tableName = prop.name;
	ret->relationColumnName = relationColumn;
	return ret->relationTableColumn.GetNumColumn();
}

int MugenScript::MugenScriptDatabaseTableRelationEx::ReplaceRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName)
{
	auto& relColumn = relation->relationTableColumn;
	if (column->GetType(relation->columnID) != relColumn.GetType(relation->relationColumn))
		return -1;
	DeleteRelation();

	auto path = relDatabaseName + ".mdb";
	MugenScriptDatabaseHandle dbHandle;
	MugenScriptDatabaseFile file;
	MugenScriptDatabaseTableProp prop;
	MugenScriptDatabaseReader reader;
	file.open(path);
	int tablePos = GetTablePos(relDatabaseName, relTableName, file);
	if (tablePos == -1)
	{
		file.close();
		return -1;
	}
	reader.ReadTableHandle(prop, tablePos, file);
	RelationData* ret = new RelationData(columnID, relationColumn, tablePos, prop, file);
	if (ret->relationColumn == -1)
	{
		delete ret;
		file.close();
		return -1;
	}
	file.close();
	relation = ret;
	//auto& tableVal = relationTableData[columnID];

	ret->databaseName = relDatabaseName;
	ret->tableName = prop.name;
	ret->relationColumnName = relationColumn;
	return ret->relationTableColumn.GetNumColumn();
}

void MugenScript::MugenScriptDatabaseTableRelationEx::DeleteRelation()
{
	delete relation;
	relation = nullptr;
}

int MugenScript::MugenScriptDatabaseTableRelationEx::ReadRelation(int id, int dstBuff, MugenScriptDatabaseValue* dst)
{
	if (!relation)
		return -1;
	TableData data;
	int keyIdx = 0;
	data = id;
	auto n = relationTableData.find(keyIdx, data);
	if (!n)
		return -1;
	int numColumn = relation->relationTableColumn.GetNumColumn();
	auto record = relation->GetRelation(n->keys[keyIdx].value);
	if (!record)
		return -1;
	for (int j = 0; j < dstBuff && j < numColumn; ++j)
		dst[j] = record[j];
	return 0;
}

MugenScript::MugenScriptDatabaseTableRelationEx::RelationData* MugenScript::MugenScriptDatabaseTableRelationEx::Relation()
{
	return relation;
}

MugenScript::MugenScriptDatabaseTableRelationEx::RelationData* MugenScript::MugenScriptDatabaseTableRelationEx::Relation() const
{
	return relation;
}

void MugenScript::MugenScriptDatabaseTableRelationEx::UpdateReference(const char* tableStart, const MugenScriptDatabaseIndex& idx)
{
	if (!relation || column->GetType(relation->columnID) != DATABASE_VALUE_TYPE_EX_REL)
		return;
	auto read = tableStart;
	int columnID = relation->columnID;
	MugenScriptDatabaseRecord record(column);
	for (int i = 0; i < idx.Size(); ++i)
	{
		int id = 0, pos = 0;
		idx.ReadFromIndex(i, id, pos);
		if (id == -1 || pos == -1)
			continue;
		record.CollectionMemory(&read[pos]);
		TableData data;
		data = id;
		int keyIdx = 0;
		auto n = relationTableData.find(keyIdx, data);
		int relatedID = record.Get()[columnID].value.ivalue;
		if (!n || relatedID == -1)
			continue;
		auto relatedRec = relation->GetRelation(relatedID);
		n->keys[keyIdx].value = relatedRec[columnID];
	}
}

void MugenScript::MugenScriptDatabaseTableRelationEx::CopyRelatedData()
{
	if (!relation)
		return;

	int numRecord = relation->relationTableIndex.Size();
	for (int i = 0; i < numRecord && i < relationTableData.size(); ++i)
	{
		int id = 0, pos = 0;
		relation->relationTableIndex.ReadFromIndex(i, id, pos);
		relationTableData[i].value = relation->GetRelation(id)[columnID];
	}
}

int MugenScript::MugenScriptDatabaseTableRelationEx::SizeOfDeployment() const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	int idx = 0;

	if (relation)
	{
		MugenScriptDatabaseValue vColumnID;
		MugenScriptDatabaseValue vDatabaseName;
		MugenScriptDatabaseValue vTableName;
		MugenScriptDatabaseValue vColumnRel;
		vColumnID = relation->columnID;
		vDatabaseName = relation->databaseName.C_Str();
		vTableName = relation->tableName.C_Str();
		vColumnRel = relation->relationColumnName.C_Str();

		idx += vColumnID.size();
		idx += vDatabaseName.size();
		idx += vTableName.size();
		idx += vColumnRel.size();
		idx += sizeof(int);
	}
	else
		idx += sizeof(int);

	MugenScriptDatabaseValue vSize;
	auto& tableData = relationTableData;

	vSize = tableData.size();
	idx += vSize.size();
	for (int j = 0; j < tableData.size(); ++j)
	{
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vValue;

		idx += sizeof(int);
		idx += sizeof(DATABASE_VALUE_TYPE);
		idx += tableData[j].value.size();
	}

	return idx;
}

int MugenScript::MugenScriptDatabaseTableRelationEx::DeployMemory(char* dst, int capacity) const
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	int idx = 0;

	if (relation)
	{
		MugenScriptDatabaseValue vColumnID;
		MugenScriptDatabaseValue vDatabaseName;
		MugenScriptDatabaseValue vTableName;
		MugenScriptDatabaseValue vColumnRelName;
		MugenScriptDatabaseValue vColumnRelType;
		vColumnID = columnID;
		vDatabaseName = relation->databaseName.C_Str();
		vTableName = relation->tableName.C_Str();
		vColumnRelName = relation->relationColumnName.C_Str();
		vColumnRelType = (int)relation->relationTableColumn.GetType(relation->relationColumn);

		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vDatabaseName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vTableName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnRelName);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vColumnRelType);
	}
	else
	{
		MugenScriptDatabaseValue vInvalid;
		vInvalid = -1;
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vInvalid);
	}

	auto& tableData = relationTableData;

	MugenScriptDatabaseValue vSize;
	vSize = tableData.size();
	idx += memio.WriteValue(&dst[idx], capacity - idx, &vSize);
	for (int j = 0; j < tableData.size(); ++j)
	{
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vType;
		MugenScriptDatabaseValue vValue;
		vID = tableData[j].id;
		vValue = tableData[j].value;
		vType = (int)vValue.type;
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vType);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &vValue);
	}


	return idx;
}

int MugenScript::MugenScriptDatabaseTableRelationEx::CollectionMemory(const char* src)
{
	bool isRelationExist = true;
	MugenScriptDatabaseReader reader;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	auto read = src;
	MugenScriptDatabaseValue vColumnID;
	MugenScriptDatabaseValue vDatabaseName;
	MugenScriptDatabaseValue vTableName;
	MugenScriptDatabaseValue vColumnRel;
	MugenScriptDatabaseValue vColumnRelType;
	MugenScriptDatabaseFile file;
	MugenScriptDatabaseTableProp tableProp;
	MugenScriptDatabaseColumn relColumn(0, 0, 0, 0);
	Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> map;
	MugenScriptDatabaseTableMapData data;
	int keyIdx = 0;

	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnID, read);
	if (vColumnID.value.ivalue != -1)
	{
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vDatabaseName, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vTableName, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vColumnRel, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vColumnRelType, read);

		Mugenshuen::string_t path = vDatabaseName.value.string;
		if (path != "")
			path += ".mdb";
		else
			isRelationExist = false;

		if (file.exist(path) && isRelationExist)
			file.open(path);
		else
			isRelationExist = false;

		if (file.is_loaded() && isRelationExist)
			reader.ReadDatabaseMap(map, file);
		else
			isRelationExist = false;

		data.name = vTableName.value.string;
		auto n = map.find(keyIdx, data);
		if (isRelationExist && n)
		{
			reader.ReadTableHandle(tableProp, n->keys[keyIdx].pos, file);
			reader.ReadTableColumn(n->keys[keyIdx].pos, tableProp, relColumn, file);
			int relColumnID = relColumn.Find(vColumnRel.value.string);
			if (relColumnID != -1 && relColumn.GetType(relColumnID) == vColumnRelType.value.ivalue)
			{
				relation = new RelationData(columnID, vColumnRel.value.string, n->keys[keyIdx].pos, tableProp, file);
				relation->databaseName = vDatabaseName.value.string;
				relation->tableName = vTableName.value.string;
				relation->relationColumnName = vColumnRel.value.string;
			}
		}
		file.close();
	}

	MugenScriptDatabaseValue vSize;
	auto& tableData = relationTableData;

	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, read);
	
	for (int j = 0; j < vSize.value.ivalue; ++j)
	{
		TableData data;
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vType;
		MugenScriptDatabaseValue vValue;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vType, read);
		read = memio.ReadValue((DATABASE_VALUE_TYPE)vType.value.ivalue, vValue, read);
		data.id = vID.value.ivalue;
		data.value = vValue;
		tableData.insert(data);
	}
	return read - src;
}

void MugenScript::MugenScriptDatabaseTableRelationEx::CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	this->columnID = columnID;
}

MugenScript::MugenScriptDatabaseTableRelationEx::~MugenScriptDatabaseTableRelationEx()
{
	delete relation;
}

const MugenScript::MugenScriptDatabaseValue MugenScript::MugenScriptDatabaseTableRelationEx::GetTableDataValue(int id) const
{
	TableData data;
	int keyIdx = 0;
	data = id;
	auto n = relationTableData.find(data);
	if (n)
		return n->keys[keyIdx].value;
	return MugenScriptDatabaseValue((MugenScript::DATABASE_VALUE_TYPE)MugenScript::DATABASE_VALUE_TYPE_INVALID);
}

int MugenScript::MugenScriptDatabaseTableRelationEx::GetTablePos(const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName, const MugenScriptDatabaseFile& file) const
{
	Mugenshuen::string_t path;
	MugenScriptDatabaseReader reader;
	int keyIdx = 0;
	MugenScriptDatabaseTableMapData mapData;
	Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> map;
	path = dbName + ".mdb";
	if (!file.is_loaded())
		return -1;
	reader.ReadDatabaseMap(map, file);
	mapData.name = talbeName;
	auto n = map.find(keyIdx, mapData);
	if (!n)
		return -1;
	return n->keys[keyIdx].pos;
}


MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::GetRelation(int recordID)
{
	if (recordID == -1)
		return nullptr;
	int pos = 0;
	MugenScriptDatabaseReader reader;
	reader.ReadTableRecord(tableStart, recordID, relationTableIndex, relationTableRecord, file);

	return relationTableRecord.Get();
}
MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::GetRelation(const MugenScriptDatabaseValue& recordID)
{
	int id = 0;
	return GetRelation(id,recordID);
}
MugenScript::MugenScriptDatabaseValue* MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::GetRelation(int& id,const MugenScriptDatabaseValue& recordID)
{
	MugenScriptDatabaseReader reader;
	relationTableColumnIndex.Read(relationColumn, recordID, 1, &id);
	if (id == -1)
		return nullptr;
	reader.ReadTableRecord(tableStart, id, relationTableIndex, relationTableRecord, file);
	return relationTableRecord.Get();
}

void MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::SetRelation(int recordID, const MugenScriptDatabaseValue* record)
{
	MugenScriptDatabaseWriter writer;
	GetRelation(recordID);
	int preSize = relationTableRecord.SizeOfDeployment();
	relationTableRecord.Set(recordID, relationTableColumn.GetNumColumn(), record);
	
	assert(preSize == relationTableRecord.SizeOfDeployment());
	writer.WriteRecord(tableStart, recordID, relationTableIndex, relationTableRecord, file);
}

MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::RelationData(int columnID, const Mugenshuen::string_t relationColumn, int tableStart, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseFile& file) :
	columnID(columnID),
	tableStart(tableStart),
	relationColumn(),
	file(),
	relationTableColumn(0, nullptr, nullptr, nullptr),
	relationTableColumnIndex(&relationTableColumn),
	relationTableIndex(),
	relationTableRecord(&relationTableColumn)
{
	MugenScriptDatabaseReader reader;
	auto start = file.distance();
	reader.ReadTableColumn(tableStart, prop, relationTableColumn, file);
	reader.ReadTableIndex(tableStart, prop, relationTableIndex, file);

	relationTableRecord.UpdateColumnData();
	relationTableColumnIndex.UpdateColumnData();
	relationTableColumnIndex.CreateColumnIndex(relationTableColumn.Find(relationColumn), relationTableIndex, tableStart, file);
	 
	this->relationColumn = relationTableColumn.Find(relationColumn);
	if (this->relationColumn == -1)
		return;
	this->file.open(file.name());
}


MugenScript::MugenScriptDatabaseTableRelationEx::RelationData::~RelationData()
{
	if (file.is_loaded())
		file.close();
}

void MugenScript::MugenScriptDatabaseIO::ReadDatabaseHandle(MugenScriptDatabaseHandle& dst)
{
	SeekDatabaseHead();
	dst = *(MugenScriptDatabaseHandle*)GetReadPoint();
}

void MugenScript::MugenScriptDatabaseIO::ReadTableHandle(MugenScriptDatabaseTableProp& dst)
{
	SeekTableHead();
	dst = *(MugenScriptDatabaseTableProp*)GetReadPoint();
}

void MugenScript::MugenScriptDatabaseIO::ReadTableColumn(MugenScriptDatabaseColumn& dst)
{
	SeekColumn();
	dst.CollectionMemory(GetReadPoint());
	EndRead();
}

void MugenScript::MugenScriptDatabaseIO::ReadTableRecord(const int id, MugenScriptDatabaseRecord& dst)
{
	SeekRecord(id);
	dst.CollectionMemory(GetReadPoint());
	EndRead();
}

void MugenScript::MugenScriptDatabaseIO::WriteTableRecord(const int id, const MugenScriptDatabaseRecord& src)
{
	auto size = src.SizeOfDeployment();
	SeekRecord(id);
	src.DeployMemory(GetWritePoint(size), size);
}

int MugenScript::MugenScriptDatabaseScriptAlias::Create(const int id, const int column, MugenScriptDatabaseValue* value)
{
	AliasData data;
	AliasIndex indexData;
	data = id;
	data.value = value[column];
	indexData.value = value[column];
	if (map.find(data)||index.find(indexData))
		return -1;
	map.insert(data);
	index.insert(indexData);
	return 0;
}

int MugenScript::MugenScriptDatabaseScriptAlias::Delete(const int id)
{
	AliasData data;
	AliasIndex indexData;
	int keyIndex = 0;
	data = id;
	auto n = map.find(keyIndex, data);
	if (!n)
		return -1;
	indexData.value = n->keys[keyIndex].value;
	index.remove(indexData);
	map.remove(data);
	return 0;
}

void MugenScript::MugenScriptDatabaseScriptAlias::Read(const int id, const int column, MugenScriptDatabaseValue* dst) const
{
	AliasData data;
	int keyIndex = 0;
	data = id;
	auto n = map.find(keyIndex,data);
	if (!n)
	{
		dst[column]="";
		return;
	}
	dst[column] = n->keys[keyIndex].value;
}

void MugenScript::MugenScriptDatabaseScriptAlias::Replace(const int id, const int column, MugenScriptDatabaseValue* value)
{
	Delete(id);
	Create(id, column, value);
}

int MugenScript::MugenScriptDatabaseScriptAlias::SizeOfDeployment() const
{
	int size = 0;
	size += sizeof(int);
	for (int i = 0; i < map.size(); ++i)
	{
		size += sizeof(int);
		size += map[i].value.size();
	}
	size += sizeof(int);
	for (int i = 0; i < index.size(); ++i)
	{
		size += sizeof(int);
		size += index[i].value.size();
	}
	return size;
}

int MugenScript::MugenScriptDatabaseScriptAlias::DeployMemory(char* dst, int capacity) const
{
	int idx = 0;
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vSize(map.size());
	idx += memio.WriteValue(&dst[idx], capacity - idx, &vSize);

	for (int i = 0; i < vSize.value.ivalue; ++i)
	{
		auto& data = map[i];
		MugenScriptDatabaseValue vID(data.id);

		idx += memio.WriteValue(&dst[idx], capacity - idx, &vID);
		idx += memio.WriteValue(&dst[idx], capacity - idx, &data.value);
	}

	MugenScriptDatabaseValue vSizeIndex(index.size());
	idx += memio.WriteValue(&dst[idx], capacity - idx, &vSizeIndex);

	for (int i = 0; i < vSizeIndex.value.ivalue; ++i)
	{
		memio.WriteValue(&dst[idx], capacity - idx, &index[i].value);
	}

	return idx;
}

int MugenScript::MugenScriptDatabaseScriptAlias::CollectionMemory(const char* src)
{
	MugenScriptDatabaseTableDataMemWriterReader memio;
	MugenScriptDatabaseValue vSize;
	auto read = src;
	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, read);
	for (int i = 0; i < vSize.value.ivalue; ++i)
	{
		AliasData data;
		MugenScriptDatabaseValue vID;
		MugenScriptDatabaseValue vValue;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vID, read);
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, vValue, read);
		data.id = vID.value.ivalue;
		data.value = vValue;
		map.insert(data);
	}

	read = memio.ReadValue(DATABASE_VALUE_TYPE_INT, vSize, read);
	for (int i = 0; i < vSize.value.ivalue; ++i)
	{
		AliasIndex data;
		read = memio.ReadValue(DATABASE_VALUE_TYPE_STR, data.value, read);
		index.insert(data);
	}
	return read - src;
}

void MugenScript::MugenScriptDatabaseScriptAlias::CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)
{
	column = columnID;
	this->numColumn = numColumn;
}
