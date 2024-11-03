#pragma once
#include "Container.h"
#include "HashString.h"
#define UNIQUE_ID_SIZE 16

class UniqueID;

class UniqueIDTable
{
	Container::AsyncHashMap<size_t, UniqueID*> idMap;
public:
	void Regist(UniqueID*);
	bool Find(UniqueID*);
	void Erase(UniqueID*);
	static UniqueIDTable& instance();
};

class UniqueID
{
	char uid[UNIQUE_ID_SIZE];
public:

	size_t hash()const;
	void generate();
	void clear();
	bool same(const UniqueID&)const;
	bool valid()const;
	bool operator==(const UniqueID&)const;
	bool operator!=(const UniqueID&)const;
	bool operator=(const UniqueID&) = delete;
	operator bool()const;
	operator char* ();
	UniqueID(const UniqueID&) = delete;
	UniqueID();
	~UniqueID();
};

typedef UniqueID uid_t;