#pragma once
#include "Mugenshuen.h"

namespace MugenScript
{
	using ConditionCommand = unsigned int;
	using MugenScriptEventConditionFlags = unsigned int;
	using DATABASE_VALUE_TYPE = unsigned int;
	class MugenScriptDatabaseTable;
	class MugenScriptDatabaseTableEx;

	enum DATABASE_VALUE_TYPE_
	{
		DATABASE_VALUE_TYPE_INT,
		DATABASE_VALUE_TYPE_DBL,
		DATABASE_VALUE_TYPE_STR,
		DATABASE_VALUE_TYPE_NUM_TYPE,
		DATABASE_VALUE_TYPE_INVALID = -1
	};

	enum DATABASE_VALUE_TYPE_EX
	{
		DATABASE_VALUE_TYPE_EX_REL = DATABASE_VALUE_TYPE_NUM_TYPE,
		DATABASE_VALUE_TYPE_EX_TRIGGER,
		DATABASE_VALUE_TYPE_EX_EVENT,
		DATABASE_VALUE_TYPE_EX_SCRIPT,
		DATABASE_VALUE_TYPE_EX_NUM_TYPE,
		DATABASE_VALUE_TYPE_EX_ALIAS
	};

	enum DATABASE_PAGE_SIZE
	{
		DATABASE_PAGE_SIZE_1024 = 1024,
		DATABASE_PAGE_SIZE_2048 = 2048,
		DATABASE_PAGE_SIZE_4096 = 4096,
		DATABASE_PAGE_SIZE_8192 = 8192,
	};

	enum MugenScriptTriggerBehavior_
	{
		MugenScriptTriggerBehavior_CondRelationTrue = 1 << 0,
		MugenScriptTriggerBehavior_CondRelationFalse = 1 << 1,
		MugenScriptTriggerBehavior_CondEdge = 1 << 2,
		MugenScriptTriggerBehavior_CondEdgeUp = 1 << 3,
		MugenScriptTriggerBehavior_CondEdgeDown = 1 << 4,
		MugenScriptTriggerBehavior_ActOne = 1 << 5,
		MugenScriptTriggerBehavior_AlwaysTrue = 1 << 6,
		MugenScriptTriggerBehavior_AlwaysFalse = 1 << 7,
		MugenScriptTriggerBehavior_IgEvent = 1 << 30,
		MugenScriptTriggerBehavior_TriggerFlag = 1 << 31
	};

	enum MugenScriptEventConditionFlags_
	{
		MugenScriptEventConditionFlags_1 = 1 << 0,
		MugenScriptEventConditionFlags_2 = 1 << 1,
		MugenScriptEventConditionFlags_3 = 1 << 2,
		MugenScriptEventConditionFlags_4 = 1 << 3,
		MugenScriptEventConditionFlags_5 = 1 << 4,
		MugenScriptEventConditionFlags_6 = 1 << 5,
		MugenScriptEventConditionFlags_7 = 1 << 6,
		MugenScriptEventConditionFlags_8 = 1 << 7,
		MugenScriptEventConditionFlags_9 = 1 << 8,
		MugenScriptEventConditionFlags_10 = 1 << 9,
		MugenScriptEventConditionFlags_Num = 10,
		MugenScriptEventConditionFlags_And = 1 << 10,
	};

	class MugenScriptDatabaseValue
	{
	public:

		union TableValue
		{
			int ivalue;
			double dvalue;
			char* string;
			//void* userData;
			TableValue(int i) :ivalue(i) {}
			TableValue(double d) :dvalue(d) {}
			TableValue(char* s) :string(s) {}
			TableValue() :dvalue() {}
			~TableValue() {}
		};

		TableValue value;
		DATABASE_VALUE_TYPE type;
		unsigned int userData;

		bool operator<=(const MugenScriptDatabaseValue& o)const;
		bool operator>=(const MugenScriptDatabaseValue& o)const;
		bool operator<(const MugenScriptDatabaseValue& o)const;
		bool operator>(const MugenScriptDatabaseValue& o)const;
		bool operator==(const MugenScriptDatabaseValue& o)const;
		bool operator!=(const MugenScriptDatabaseValue& o)const;

		void operator=(const MugenScriptDatabaseValue&);
		void operator=(MugenScriptDatabaseValue&&);
		int size()const;
		MugenScriptDatabaseValue() :value(0.0), type(DATABASE_VALUE_TYPE_INT),userData() {}
		MugenScriptDatabaseValue(int ivalue) :value(ivalue), type(DATABASE_VALUE_TYPE_INT),userData() {};
		MugenScriptDatabaseValue(double dvalue) :value(dvalue), type(DATABASE_VALUE_TYPE_DBL),userData() {};
		MugenScriptDatabaseValue(const char* string);
		MugenScriptDatabaseValue(DATABASE_VALUE_TYPE type);
		MugenScriptDatabaseValue(const MugenScriptDatabaseValue&);
		MugenScriptDatabaseValue(MugenScriptDatabaseValue&&);
		~MugenScriptDatabaseValue();

	private:
		bool QueryDataTypeFunctionCompare(
			const MugenScriptDatabaseValue& o,
			bool(*ifunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&),
			bool(*dfunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&),
			bool(*sfunc)(const MugenScriptDatabaseValue&, const MugenScriptDatabaseValue&)
		)const;
	};

	class MugenScriptDatabaseFile
	{
	public:
		using file_t = FileSystem::FileManager::FileIO;

		bool exist(const Mugenshuen::string_t& fileName)const;
		void open(const Mugenshuen::string_t& fileName);
		void remove(const Mugenshuen::string_t& fileName)const;
		void close();
		void read(void* dst, size_type size)const;
		void write(void* src, size_type size);
		void gets(char* dst, size_type buffer)const;
		void begin()const;
		void seek_begin(size_type size)const;
		void seek_current(size_type size)const;
		int distance()const;
		int size()const;
		int lens()const;
		bool is_loaded()const;
		Mugenshuen::string_t name()const;
		Mugenshuen::string_t path()const;
		Mugenshuen::string_t stem()const;

	private:
		Mugenshuen::string_t fileName;
		file_t file;
	};


	class MugenScriptDatabaseTableDataMemWriterReader
	{
	public:

		int WriteValue(char* dst, int buffDst, const MugenScriptDatabaseValue* value)const;
		const char* ReadValue(DATABASE_VALUE_TYPE type, MugenScriptDatabaseValue& dst, const char* src)const;

	private:
		char* WriteMem(char* dst, int buffDst, const void* src, int size)const;
	};

	class MugenScriptDatabaseTableDataFileReaderWriter
	{
	public:
		using file_t = FileSystem::FileManager::FileIO;
		void ReadValue(DATABASE_VALUE_TYPE type, MugenScriptDatabaseValue& dst, MugenScriptDatabaseFile& file)const;
		int WriteValue(MugenScriptDatabaseValue* src, MugenScriptDatabaseFile& file)const;
	};

	struct MugenScriptDatabaseTableMapData
	{
		Mugenshuen::string_t name;
		int pos;
		bool operator<(const MugenScriptDatabaseTableMapData& o)const { return name < o.name; };
		bool operator>(const MugenScriptDatabaseTableMapData& o)const { return name > o.name; };
		bool operator<=(const MugenScriptDatabaseTableMapData& o)const { return name <= o.name; };
		bool operator>=(const MugenScriptDatabaseTableMapData& o)const { return name >= o.name; };
		bool operator==(const MugenScriptDatabaseTableMapData& o)const { return name == o.name; };
		void operator=(const MugenScriptDatabaseTableMapData& o) { name = o.name; pos = o.pos; };

	};

	struct MugenScriptDatabaseHandle
	{
		DATABASE_PAGE_SIZE pageSize;
		int numTable;
		int mapStart;
		int size;
	};

	struct MugenScriptDatabaseTableProp
	{
		int numExtent;
		int numRecord;
		int startRecord;
		int startColumn;
		int startIndex;
		int startColumnIndex;
		int nextID;
		int tableSize;
		DATABASE_PAGE_SIZE pageSize;
		char name[1024 - sizeof(int) * 9];
	};



	class IMugenScriptDatabaseFeature
	{
	public:

		virtual int Create(const int id, const int column, MugenScriptDatabaseValue* value) = 0;
		virtual int Delete(const int id) = 0;
		virtual void Read(const int id,const int column, MugenScriptDatabaseValue* dst)const = 0;
		virtual  void Replace(const int id,const int column, MugenScriptDatabaseValue* value) = 0;
		virtual void Update() = 0;
		virtual int SizeOfDeployment()const = 0;
		virtual int DeployMemory(char* dst, int capacity)const = 0;
		virtual int CollectionMemory(const char* src) = 0;
		virtual void CreateFeatrue(int numColumn,int columnID, const Mugenshuen::string_t* name,const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials) = 0;
		void SetTableName(const Mugenshuen::string_t&);
		void SetDatabaseName(const Mugenshuen::string_t&);
		virtual ~IMugenScriptDatabaseFeature(){}
	protected:

		Mugenshuen::string_t table;
		Mugenshuen::string_t database;
	};

	class MugenScriptDatabaseColumn
	{
	public:

		const DATABASE_VALUE_TYPE GetType(int index)const;
		const DATABASE_VALUE_TYPE* GetTypes()const;

		const MugenScriptDatabaseValue& GetInitial(int index)const;
		const MugenScriptDatabaseValue* GetInitials()const;

		const MugenScriptDatabaseValue& GetName(int index)const;
		const MugenScriptDatabaseValue* GetNames()const;

		int Find(const Mugenshuen::string_t)const;

		int GetNumColumn()const;

		int SizeOfDeployment()const;
		int DeployMemory(char* dst, int capacity)const;
		int CollectionMemory(const char* src);
		int CollectionFile(MugenScriptDatabaseFile&);
		void Clear();

		void operator=(const MugenScriptDatabaseColumn&);
		void operator=(MugenScriptDatabaseColumn&&);
		MugenScriptDatabaseColumn(int numColumn, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* type, const MugenScriptDatabaseValue* initial);
		MugenScriptDatabaseColumn(const MugenScriptDatabaseColumn&);
		MugenScriptDatabaseColumn(MugenScriptDatabaseColumn&&);
		~MugenScriptDatabaseColumn();

	private:

		int numColumn;
		DATABASE_VALUE_TYPE* type;
		MugenScriptDatabaseValue* name;
		MugenScriptDatabaseValue* initial;
	};

	class MugenScriptDatabaseRecord
	{
	public:

		void UpdateColumnData();
		void Set(int id, const int num, const MugenScriptDatabaseValue* value);
		MugenScriptDatabaseValue* Get()const;
		void Delete(char* dst);
		int ID()const;
		int NumColumn()const;
		int SizeOfDeployment()const;
		int DeployMemory(char* dst, int capacity)const;
		int CollectionMemory(const char* src)const;
		int CollectionFile(MugenScriptDatabaseFile&)const;
		int CollectionSizeRecord(const char* src)const;
		int CollectionID(const char*)const;

		MugenScriptDatabaseRecord(const MugenScriptDatabaseColumn* column);
		MugenScriptDatabaseRecord(const MugenScriptDatabaseRecord&) = delete;
		MugenScriptDatabaseRecord(MugenScriptDatabaseRecord&&) = delete;
		~MugenScriptDatabaseRecord();

	private:
		int numColumn;
		int id;
		MugenScriptDatabaseValue* value;
		const MugenScriptDatabaseColumn* column;

	};

	class MugenScriptDatabaseIndex
	{
	public:

		struct Index
		{
			int id;
			int pos;
			operator int()const { return id; }
			void operator=(int v) { id = v; }
		};

		bool Create(int id, int pos);
		void Read(int id, int& pos)const;
		void Delete(int id);
		void Clear();
		void ReadFromIndex(int idx, int& id, int& pos)const;
		int Size()const;

		int SizeOfDeployment()const;
		int DeployMemory(char* dst, int capacity)const;
		int CollectionMemory(const char* src);
		int CollectionFile(MugenScriptDatabaseFile&);

		void operator=(const MugenScriptDatabaseIndex&);
		void operator=(MugenScriptDatabaseIndex&&);

		MugenScriptDatabaseIndex() {}
		MugenScriptDatabaseIndex(const MugenScriptDatabaseIndex&);
		MugenScriptDatabaseIndex(MugenScriptDatabaseIndex&&);
		~MugenScriptDatabaseIndex() {}

	private:

		Mugenshuen::btree_t<Index> idx;
	};

	class MugenScriptDatabaseColumnIndex
	{
	public:
		struct ColumnIndexData
		{
			int id;
			MugenScriptDatabaseValue value;

			bool operator<=(const ColumnIndexData& o)const;
			bool operator>=(const ColumnIndexData& o)const;
			bool operator<(const ColumnIndexData& o)const;
			bool operator>(const ColumnIndexData& o)const;
			bool operator==(const ColumnIndexData& o)const;
			bool operator!=(const ColumnIndexData& o)const;

		};

		struct ColumnIndex
		{
			int columnID;
			Mugenshuen::btree_t<ColumnIndexData> indexData;
		};

		void CreateColumnIndex(int columnID, const MugenScriptDatabaseIndex& idx, const char* tableHead);
		void CreateColumnIndex(int columnID, const MugenScriptDatabaseIndex& idx,int tableStart, MugenScriptDatabaseFile& file);
		
		bool Create(int id, const MugenScriptDatabaseValue*value);
		void Read(int columnID,const MugenScriptDatabaseValue& val,int dstBuf, int id[])const;
		void Delete(int id,const MugenScriptDatabaseValue* val);
		void Clear();
		void ClearColumnIndex(int columnID);
		void Replace(int id,const MugenScriptDatabaseValue* val);
		void UpdateColumnData();
		int Count(int columnID, const MugenScriptDatabaseValue& val)const;

		int SizeOfDeployment()const;
		int DeployMemory(char* dst, int capacity)const;
		int CollectionMemory(const char* src);
		int CollectionFile(MugenScriptDatabaseFile&);

		MugenScriptDatabaseColumnIndex(const MugenScriptDatabaseColumn*);
		MugenScriptDatabaseColumnIndex(const MugenScriptDatabaseColumnIndex&) = delete;
		MugenScriptDatabaseColumnIndex(MugenScriptDatabaseColumnIndex&&) = delete;
		~MugenScriptDatabaseColumnIndex() {};
	private:
		const MugenScriptDatabaseColumn* column;
		Mugenshuen::vector_t<ColumnIndex> columnIndexData;
	};

	class MugenScriptDatabaseRelation
	{
	public:

		struct TableData
		{
			int id;
			MugenScriptDatabaseValue value;
			operator int()const { return id; }
			void operator=(const int v) { id = v; }
		};

		struct RelationData
		{
			int columnID;
			int tableStart;
			int relationColumn;
			Mugenshuen::string_t databaseName;
			Mugenshuen::string_t tableName;
			Mugenshuen::string_t relationColumnName;
			MugenScriptDatabaseFile file;
			MugenScriptDatabaseColumn relationTableColumn;
			MugenScriptDatabaseIndex relationTableIndex;
			MugenScriptDatabaseColumnIndex relationTableColumnIndex;
			MugenScriptDatabaseRecord relationTableRecord;
			MugenScriptDatabaseValue* GetRelation(int recordID);
			MugenScriptDatabaseValue* GetRelation(const MugenScriptDatabaseValue& recordID);
			RelationData(int columnID, const Mugenshuen::string_t relationColumn, int tableStart, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseFile& file);
			~RelationData();
		};

		typedef MugenScriptDatabaseValue*(RelationData::* func_t)(int);
		using delegate_t = Mugenshuen::delegate_t<func_t>;

		int Create(int id, MugenScriptDatabaseValue* value);
		void Delete(int id);
		void Read(int id,MugenScriptDatabaseValue* dst)const;
		void Replace(int id, MugenScriptDatabaseValue*);
		int CreateRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName,Mugenshuen::string_t relTableName);
		int ReplaceRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName);
		void DeleteRelation(int columnID);
		int ReadRelation(int id,int columnID,int dstBuff, MugenScriptDatabaseValue* dst);
		RelationData* Relation(int columnID);
		const RelationData* Relation(int columnID)const;
		void UpdateColumnData();
		void UpdateReference(const char* tableStart,const MugenScriptDatabaseIndex& idx);

		int SizeOfDeployment()const;
		int DeployMemory(char* dst, int capacity)const;
		int CollectionMemory(const char* src);
		//int CollectionFile(MugenScriptDatabaseFile&);

		MugenScriptDatabaseRelation(MugenScriptDatabaseColumn* column):column(column),relationList(column->GetNumColumn()),relationTableData(column->GetNumColumn()) {}
		~MugenScriptDatabaseRelation();
	private:
		int GetTablePos(const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName,const MugenScriptDatabaseFile& file)const;

		MugenScriptDatabaseColumn* column;
		Mugenshuen::vector_t<RelationData*> relationList;
		Mugenshuen::vector_t<Mugenshuen::btree_t<TableData>> relationTableData;
	};


	class MugenScriptDatabaseTable
	{
	public:

		const Mugenshuen::string_t Name()const;
		virtual int Create(int num, const MugenScriptDatabaseValue* value);
		virtual void Delete(int id);
		virtual int Read(int id, MugenScriptDatabaseValue* dst)const;
		virtual void Replace(int id, const MugenScriptDatabaseValue* value);
		virtual void Update();

		int CreateIndex(const Mugenshuen::string_t columnName);
		int ReadIndex(const Mugenshuen::string_t columnName,const MugenScriptDatabaseValue value,int dstBuf,int ids[])const;
		int NumColumnValue(const Mugenshuen::string_t columnName, const MugenScriptDatabaseValue value)const;

		virtual int SizeOfDeploymentTable()const;
		virtual int Load(MugenScriptDatabaseFile& file);
		virtual int Save(MugenScriptDatabaseFile& file);

		void CopyColumn(MugenScriptDatabaseColumn&)const;
		void CopyIndex(MugenScriptDatabaseIndex&)const;
		const int NumRecord()const;
		const int NumColumn()const;
		const Mugenshuen::string_t NameColumn(int id)const;
		const DATABASE_VALUE_TYPE* TypesColumn()const;
		const MugenScriptDatabaseValue* NamesColumn()const;
		const MugenScriptDatabaseValue* InitialsColumn()const;
		const int IDFromIndex(int idx)const;
		const int FindColumn(Mugenshuen::string_t)const;

		MugenScriptDatabaseTable();
		MugenScriptDatabaseTable(const Mugenshuen::string_t name);
		MugenScriptDatabaseTable(const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials);
		MugenScriptDatabaseTable(const MugenScriptDatabaseTable&) = delete;
		MugenScriptDatabaseTable(MugenScriptDatabaseTable&&) = delete;
		virtual ~MugenScriptDatabaseTable();

	protected:

		int GetNextID()const;
		char* GetTableReserved()const;
		const int CapacityReserved()const;
		void Construct(const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials);
	private:

		void Alloc(int numExtent, DATABASE_PAGE_SIZE size);
		void Realloc(DATABASE_PAGE_SIZE size);
		void Release();

		char* page;
		MugenScriptDatabaseTableProp* prop;
		int capacity;

		MugenScriptDatabaseColumn column;
		MugenScriptDatabaseRecord record;
		MugenScriptDatabaseIndex index;
		MugenScriptDatabaseColumnIndex columnIndex;
	};


	class MugenScriptDatabaseTableScript :public IMugenScriptDatabaseFeature
	{
	public:
		int Create(const int id, const int column, MugenScriptDatabaseValue* value) override;
		int Delete(const int id) override;
		void Read(const int id,int column, MugenScriptDatabaseValue* dst)const override;
		void Replace(const int id,const int column, MugenScriptDatabaseValue* value)override;
		void Update()override;
		int SizeOfDeployment()const override;
		int DeployMemory(char* dst, int capacity)const override;
		int CollectionMemory(const char* src) override;
		void CreateFeatrue(int numColumn,int columnID, const Mugenshuen::string_t* name,const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials);

		~MugenScriptDatabaseTableScript();
	private:
		Mugenshuen::string_t GetPathScript(const Mugenshuen::string_t&);
		int scriptSize;
		Mugenshuen::string_t path;
		MugenScriptDatabaseFile file;
		MugenScriptDatabaseIndex index;
	};


	class MugenScriptDatabaseTableRelationEx :public IMugenScriptDatabaseFeature
	{
	public:

		struct TableData
		{
			int id;
			MugenScriptDatabaseValue value;
			operator int()const { return id; }
			void operator=(const int v) { id = v; }
		};

		struct RelationData
		{
			int columnID;
			int tableStart;
			int relationColumn;
			Mugenshuen::string_t databaseName;
			Mugenshuen::string_t tableName;
			Mugenshuen::string_t relationColumnName;
			MugenScriptDatabaseFile file;
			MugenScriptDatabaseColumn relationTableColumn;
			MugenScriptDatabaseIndex relationTableIndex;
			MugenScriptDatabaseColumnIndex relationTableColumnIndex;
			MugenScriptDatabaseRecord relationTableRecord;
			MugenScriptDatabaseValue* GetRelation(int recordID);
			MugenScriptDatabaseValue* GetRelation(const MugenScriptDatabaseValue& recordID);
			MugenScriptDatabaseValue* GetRelation(int& id, const MugenScriptDatabaseValue& recordID);
			void SetRelation(int recordID, const MugenScriptDatabaseValue* record);
			RelationData(int columnID, const Mugenshuen::string_t relationColumn, int tableStart, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseFile& file);
			~RelationData();
		};

		int Create(int id,int column, MugenScriptDatabaseValue* value)override;
		int Delete(int id)override;
		void Read(int id,int column, MugenScriptDatabaseValue* dst)const override;
		void Replace(int id,int column, MugenScriptDatabaseValue*)override;
		void Update()override;
		int CreateRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName);
		int ReplaceRelation(int columnID, const Mugenshuen::string_t relationColumn, const Mugenshuen::string_t relDatabaseName, Mugenshuen::string_t relTableName);
		void DeleteRelation();
		int ReadRelation(int id, int dstBuff, MugenScriptDatabaseValue* dst);
		RelationData* Relation();
		RelationData* Relation()const;
		void UpdateReference(const char* tableStart, const MugenScriptDatabaseIndex& idx);
		void CopyRelatedData();

		int SizeOfDeployment()const override;
		int DeployMemory(char* dst, int capacity)const override;
		int CollectionMemory(const char* src) override;
		void CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)override;
		~MugenScriptDatabaseTableRelationEx();
	protected:

		const MugenScriptDatabaseValue GetTableDataValue(int id)const;

	private:
		int GetTablePos(const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName, const MugenScriptDatabaseFile& file)const;

		int columnID;
		MugenScriptDatabaseColumn* column;
		RelationData* relation;
		Mugenshuen::btree_t<TableData> relationTableData;
	};

	struct MugenScriptConditionData
	{
		int id;
		ConditionCommand command;
		MugenScriptDatabaseValue prev;
		operator int()const { return id; }
		void operator=(const int v) { id = v; }
	};

	class MugenScriptDatabaseTableTrigger :public MugenScriptDatabaseTableRelationEx
	{
	public:

		int Create(const int id, const int column, MugenScriptDatabaseValue* value) override;
		int Delete(const int id) override;
		void Read(const int id, int column, MugenScriptDatabaseValue* dst)const override;
		void Replace(const int id, const int column, MugenScriptDatabaseValue* value)override;
		void Update()override;
		int SizeOfDeployment()const override;
		int DeployMemory(char* dst, int capacity)const override;
		int CollectionMemory(const char* src) override;
		void CreateFeatrue(int numColumn,int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)override;
		bool CheckTrigger(const int id);
		int GetRelationColumn()const;
		DATABASE_VALUE_TYPE GetRelationColumnType()const;
	private:
		int columnID;
		int relationColumnID;
		DATABASE_VALUE_TYPE relationType;
		Mugenshuen::vector_t<Mugenshuen::string_t> names;
		Mugenshuen::vector_t<DATABASE_VALUE_TYPE> types;
		Mugenshuen::btree_t<MugenScriptConditionData> index;
	};

	struct MugenScriptDatabaseEvent
	{
		bool CheckEventCondition(int numValue,const MugenScriptDatabaseValue* value)const;
		int id;
		Mugenshuen::string_t eventName;
		MugenScriptEventConditionFlags flags;

		operator int()const { return id; }
		void operator=(const int v) { id = v; }
	};

	class MugenScriptDatabaseTableEvent :public IMugenScriptDatabaseFeature
	{
	public:
		int Create(const int id, const int column, MugenScriptDatabaseValue* value) override;
		int Delete(const int id) override;
		void Read(const int id, const int column, MugenScriptDatabaseValue* dst)const override;
		void Replace(const int id, const int column, MugenScriptDatabaseValue* value)override;
		void Update()override;
		int SizeOfDeployment()const override;
		int DeployMemory(char* dst, int capacity)const override;
		int CollectionMemory(const char* src) override;
		void CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)override;
		bool CheckCondition(int id, int numColumn, const MugenScriptDatabaseValue* value);
		Mugenshuen::string_t GetEventName(int id);
	private:
		Mugenshuen::btree_t<MugenScriptDatabaseEvent> eventList;
	};

	class MugenScriptDatabaseScriptAlias:public IMugenScriptDatabaseFeature
	{


		struct AliasData
		{
			int id;
			MugenScriptDatabaseValue value;
			operator int()const { return id; }
			void operator=(const int v) { id = v; }
		};
		struct AliasIndex
		{
			MugenScriptDatabaseValue value;
			bool operator<=(const AliasIndex& o)const { return value <= o.value; };
			bool operator>=(const AliasIndex& o)const { return value >= o.value; };
			bool operator<(const AliasIndex& o)const { return value < o.value; };
			bool operator>(const AliasIndex& o)const { return value > o.value; };
			bool operator==(const AliasIndex& o)const { return value == o.value; };
			bool operator!=(const AliasIndex& o)const { return value != o.value; };

		};

		int Create(const int id, const int column, MugenScriptDatabaseValue* value) override;
		int Delete(const int id) override;
		void Read(const int id, const int column, MugenScriptDatabaseValue* dst)const override;
		void Replace(const int id, const int column, MugenScriptDatabaseValue* value)override;
		void Update()override {};
		int SizeOfDeployment()const override;
		int DeployMemory(char* dst, int capacity)const override;
		int CollectionMemory(const char* src) override;
		void CreateFeatrue(int numColumn, int columnID, const Mugenshuen::string_t* name, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials)override;
	private:
		int column;
		int numColumn;
		Mugenshuen::btree_t <AliasData> map;
		Mugenshuen::btree_t <AliasIndex> index;
	};

	class MugenScriptDatabaseTableEx :public MugenScriptDatabaseTable
	{
	public:
		int Create(int num, const MugenScriptDatabaseValue* value) override;
		void Delete(int id) override;
		int Read(int id, MugenScriptDatabaseValue* dst)const override;
		void Replace(int id, const MugenScriptDatabaseValue* value);
		int Load(MugenScriptDatabaseFile&)override;
		int Save(MugenScriptDatabaseFile&)override;
		int SizeOfDeploymentTable()const override;

		int CreateRelation(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, Mugenshuen::string_t relTableName, const Mugenshuen::string_t relationColumn);
		int ReplaceRelation(const Mugenshuen::string_t columnName, const Mugenshuen::string_t databaseName, Mugenshuen::string_t relTableName, const Mugenshuen::string_t relationColumn);
		int ReadRelational(int id, int columnID, int dstBuff, MugenScriptDatabaseValue* dst);
		void DeleteRelation(const Mugenshuen::string_t columnName);
		MugenScriptDatabaseTableRelationEx::RelationData* GetRelation(int columnID)const;

		int GetConditionRelationIndex(int columnID);
		bool CheckCondition(int columnID,int recordID);
		const DATABASE_VALUE_TYPE GetConditionRelatedInitial(int columnID)const;

		MugenScriptDatabaseTableEx();
		MugenScriptDatabaseTableEx(const Mugenshuen::string_t database, const Mugenshuen::string_t name, DATABASE_PAGE_SIZE page, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials);
		virtual ~MugenScriptDatabaseTableEx();
	private:

		struct MugenScriptDatabaseFeatureData
		{
			int id;
			IMugenScriptDatabaseFeature* feature;
			operator int()const { return id; }
			void operator=(int v) { id = v; }
		};

		int SizeOfDeploymentFeatures()const;
		void ConstructFeatures(const Mugenshuen::string_t database, const Mugenshuen::string_t table,int numColumn,const Mugenshuen::string_t*names,const DATABASE_VALUE_TYPE* types,const MugenScriptDatabaseValue* initials);
		Mugenshuen::vector_t<IMugenScriptDatabaseFeature*> features;
		Mugenshuen::vector_t<MugenScriptDatabaseValue> exTableRecord;
	};

	class MugenScriptDatabaseEditor
	{
	public:
		using table_t = MugenScriptDatabaseTableEx;
		using relation_t = MugenScriptDatabaseRelation;

		int CreateDatabase(Mugenshuen::string_t name, DATABASE_PAGE_SIZE page);
		void DeleteDatabase(Mugenshuen::string_t name);
		void LoadDatabase(Mugenshuen::string_t name);
		void CloseDatabsae(bool save);
		void UpdateDatabase();

		bool CreateTable(const Mugenshuen::string_t name, int numColumn, const Mugenshuen::string_t* names, const DATABASE_VALUE_TYPE* types, const MugenScriptDatabaseValue* initials);
		void DeleteTable(Mugenshuen::string_t name);
		table_t* LoadTable(Mugenshuen::string_t name);
		void SaveTable();
		void SaveTable(table_t* table);
		void CloseTable(bool save);
		void CloseTable(bool save, table_t* table);
		bool FindTable(const Mugenshuen::string_t name);
		bool FindTable(const int pos);
		
		int NumTable();
		Mugenshuen::string_t NameTable(int idx);
		Mugenshuen::string_t CurrentDatabase();

	private:

		void WriteTableMap();
		void WriteDatabaseHandle();
		void ReadTableMap();
		void ReadTableMap(const Mugenshuen::string_t databaseName, Mugenshuen::btree_t<MugenScriptDatabaseTableMapData>& dst)const;
		void SeekTableStart(int pos);
		int AlignOfExtentByte(int byteSize);
		void SaveOneTable(table_t* table);
		int SizeOfHeader()const;

		Mugenshuen::string_t database;
		MugenScriptDatabaseHandle handle;
		MugenScriptDatabaseFile file;
		Mugenshuen::vector_t<table_t*> editTables;
		Mugenshuen::btree_t<MugenScriptDatabaseTableMapData> tableMap;
		Mugenshuen::btree_t<int> positionMap;
	};

	class MugenScriptDatabaseWriter
	{
	public:
		void WriteDatabaseHandle(const MugenScriptDatabaseHandle src, MugenScriptDatabaseFile& file)const;
		void WriteDatabaseMap(const MugenScriptDatabaseHandle handle, const Mugenshuen::btree_t< MugenScriptDatabaseTableMapData>& src, MugenScriptDatabaseFile& file)const;
		void WriteTableHandle(const MugenScriptDatabaseTableProp& src, const int pos, MugenScriptDatabaseFile& file);
		void WriteRecord(const int pos, const int id, const MugenScriptDatabaseIndex& index, const MugenScriptDatabaseRecord& src, MugenScriptDatabaseFile& file);
		void SeekTableStart(int size, const MugenScriptDatabaseFile&)const;
	private:
	};

	class MugenScriptDatabaseReader
	{
	public:
		void ReadDatabaseHandle(MugenScriptDatabaseHandle& dst, const MugenScriptDatabaseFile& file)const;
		void ReadDatabaseMap(Mugenshuen::btree_t<MugenScriptDatabaseTableMapData>& dst, const MugenScriptDatabaseFile& file)const;
		void ReadTableHandle(MugenScriptDatabaseTableProp& dst, int pos, const MugenScriptDatabaseFile& file)const;
		void ReadTableColumn(const int pos, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseColumn& dst, MugenScriptDatabaseFile& file)const;
		void ReadTableIndex(const int pos, const MugenScriptDatabaseTableProp& prop, MugenScriptDatabaseIndex& dst, MugenScriptDatabaseFile& file)const;
		void ReadTableRecord(const int pos, const int id, const MugenScriptDatabaseIndex& index,const MugenScriptDatabaseColumn& column, MugenScriptDatabaseValue* dst, MugenScriptDatabaseFile& file);
		void ReadTableRecord(const int pos, const int id, const MugenScriptDatabaseIndex& index,MugenScriptDatabaseRecord& dst, MugenScriptDatabaseFile& file);
		void SeekTableStart(int size, const MugenScriptDatabaseFile&)const;
		void ReadTablePos(int& pos,const Mugenshuen::string_t dbName, const Mugenshuen::string_t talbeName, const MugenScriptDatabaseFile& file)const;
	private:
	};

	class MugenScriptDatabaseIO
	{
	public:

		virtual void Open(const Mugenshuen::string_t database) = 0;
		virtual void Close(const Mugenshuen::string_t database) = 0;
		virtual void SetDatabase(const Mugenshuen::string_t database) = 0;
		virtual void SetTable(const Mugenshuen::string_t table) = 0;
		virtual void SeekDatabaseHead() = 0;
		virtual void SeekTableHead() = 0;
		virtual void SeekColumn() = 0;
		virtual void SeekRecord(const int id) = 0;
		virtual void SeekRecord(const Mugenshuen::string_t column, const MugenScriptDatabaseValue id) = 0;

		virtual const char* GetReadPoint()const = 0;
		virtual char* GetWritePoint(int size)const = 0;
		virtual void EndRead() {}
		virtual void EndWrite(){}
		
		void ReadDatabaseHandle(MugenScriptDatabaseHandle& dst);
		void ReadTableHandle(MugenScriptDatabaseTableProp& dst);
		void ReadTableColumn(MugenScriptDatabaseColumn& dst);
		void ReadTableRecord(const int id, MugenScriptDatabaseRecord& dst);

		void WriteTableRecord(const int id, const MugenScriptDatabaseRecord& src);

	};

	class MugenScriptDatabsePicture
	{
	public:

		void Load(const Mugenshuen::string_t path);
		void Unload(const Mugenshuen::string_t path);


	};
}
