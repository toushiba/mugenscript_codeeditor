#pragma once
#include <fstream>
#include <filesystem>
#include "Container.h"

namespace FileSystem
{

	enum FILE_MODE
	{
		FILE_MODE_TEXT,
		FILE_MODE_BINARY,
		FILE_MODE_BAT
	};

	//プロジェクトファイルからの相対パス
	typedef std::filesystem::path path_t;

	bool fopen(path_t, const char* mode);
	void fclose(path_t);


	bool Exists(path_t);
	bool IsDirectory(path_t);
	bool IsRegular(path_t);
	bool IsRootDirectory(path_t);
	void Rename(path_t path, path_t newName);
	void Move(path_t src, path_t dst);
	void RemoveAll(path_t path);
	void CreateNewDirectorys(path_t path);
	void CreateNewDirectory(path_t path);
	void CreateNewDirectory(path_t path, const char* name);
	void CreateNewFile(path_t path);
	void Delete(path_t path);
	void Copy(path_t src,path_t dst);
	std::filesystem::path Absolute(path_t);
	std::filesystem::path Relative(path_t);
	std::filesystem::path Proximate(path_t);
	std::filesystem::path Extension(path_t);
	std::filesystem::path Stem(path_t);
	std::filesystem::path Parent(path_t);
	std::filesystem::path ParentDirectory(path_t);
	std::filesystem::path Current();
	std::filesystem::path FileName(path_t);
	std::filesystem::path RemoveFileName(path_t);
	std::filesystem::path InportPath(path_t);
	std::filesystem::path Root(path_t);
	size_type FileSize(path_t);


	class FileManager
	{
	public:

		struct FileData
		{
			path_t path;
			std::fstream* file;
			SpinLock* lock;
			size_type* count;
			size_type size;
			size_type* point;
			bool operator<(const FileData&)const;
			bool operator<=(const FileData&)const;
			bool operator>(const FileData&)const;
			bool operator>=(const FileData&)const;
			bool operator==(const FileData&)const;
			void operator=(const FileData&);
			void operator=(FileData&&);
			FileData() :path(), file(), lock(),count(),size(),point() { point = new size_type(); }
			FileData(const FileData&);
			FileData(FileData&&);
			~FileData() { delete point; }
		};

		class FileIO
		{
		public:
			using data_t = FileData;
			using path = std::filesystem::path;
			using it = std::filesystem::directory_iterator;

			void read(void* dest,size_type size)const;
			void readall(void* dest)const;
			void read(void* dest, size_type size,size_type offsetInByte);
			void gets(char* dst,size_type buffer)const;
			void write(void* src, size_type size);
			void write(void* src, size_type size, size_type offsetInByte);
			void add(void* src, size_type size);
			void begin()const;
			void seek_begin(size_type size)const;
			void seek_current(size_type size)const;
			size_type distance()const;
			size_type size()const;
			size_type lens()const;
			bool is_loaded()const;
			path_t get_path()const;
			
			FileIO(data_t data) :fileData(data) {};
			FileIO() :fileData() {};
			~FileIO() {};

		private:
			data_t fileData;
			friend FileManager;
		};

		class FileIT
		{
		public:
			using path = std::filesystem::path;
			using it = std::filesystem::directory_iterator;
			path get();
			bool last();
			void next();
			void entry();
			FileIT(path rootDirectory);
		private:

			path root;
			it iterator;
			it begin;
			it end;
		};

		class DirectoryIT
		{
		public:
			using path = std::filesystem::path;
			using it = std::filesystem::directory_iterator;
			path get();
			bool last();
			void next();
			void entry();
			DirectoryIT(path rootDirectory);
		private:
			path root;
			it iterator;
			it begin;
			it end;
		};

		FileIO open(path_t,FILE_MODE mode);
		void close(const FileIO&);
		FileIT file(path_t directory);
		DirectoryIT directory(path_t root);

	private:

		void CreateDirectoryPath(path_t path);
		Container::AsyncFreeListEx<std::fstream> freeList;
		Container::AsyncFreeListEx<SpinLock> lockList;
		Container::AsyncFreeListEx<size_type> openCountList;
		Container::BTree<FileData> fileMap;
	}static fm;

}