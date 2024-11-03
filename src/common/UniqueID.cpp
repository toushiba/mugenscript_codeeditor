#include "common/UniqueID.h"
#include <cassert>
#define TABLE_INIT_SIZE 256
#define RAND_MAX 0x7fffffff


size_t UniqueID::hash()const
{
	assert(valid());
	return ::hash(uid, sizeof(uid) / sizeof(uid[0]));
}

void UniqueID::generate()
{
	auto& idHashTable = UniqueIDTable::instance();

	if (valid())
		clear();

	do {
		for (int i = 0; i < sizeof(uid) / sizeof(uid[0]); i += sizeof(u32_t))
		{
			u32_t r = (u32_t)rand(RAND_MAX);
			memcpy(&uid[i], &r, sizeof(u32_t));
		}
	} while (idHashTable.Find(this));

	idHashTable.Regist(this);
}

void UniqueID::clear()
{
	auto& idHashTable = UniqueIDTable::instance();
	idHashTable.Erase(this);
	for (int i = 0; i < sizeof(uid) / sizeof(uid[0]); ++i)
		uid[i] = 0;
}

bool UniqueID::same(const UniqueID& id)const
{
	for (int i = 0; i < sizeof(uid) / sizeof(uid[0]); ++i)
		if (uid[i] != id.uid[i])
			return false;
	return true;
}

bool UniqueID::valid()const
{
	for (int i = 0; i < sizeof(uid) / sizeof(uid[0]); ++i)
		if(uid[i]!=0)
			return true;
	return false;
}

bool UniqueID::operator==(const UniqueID& id)const
{
	return same(id);
}

bool UniqueID::operator!=(const UniqueID& id)const
{
	return !same(id);
}

UniqueID::operator bool()const
{
	return valid();
}

UniqueID::operator char* ()
{
	return uid;
}

UniqueID::UniqueID():uid()
{
	generate();
}

UniqueID::~UniqueID()
{
	if (valid())
		UniqueIDTable::instance().Erase(this);
}

void UniqueIDTable::Regist(UniqueID* id)
{
	idMap[id->hash()] = id;
}

bool UniqueIDTable::Find(UniqueID* id)
{
	return idMap.find(id->hash());
}

void UniqueIDTable::Erase(UniqueID* id)
{
	idMap.erase(id->hash());
}

UniqueIDTable& UniqueIDTable::instance()
{
	static UniqueIDTable table;
	return table;
}
