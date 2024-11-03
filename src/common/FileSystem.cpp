#include <string>
#include <filesystem>
#include <windows.h>
#include "common/FileSystem.h"

bool FileSystem::fopen(path_t, const char* mode)
{
	return false;
}

bool FileSystem::Exists(path_t path)
{
	//mainがあるディレクトリからの相対パス
	std::filesystem::path p = path;
	return std::filesystem::exists(p);
}

bool FileSystem::IsDirectory(path_t path)
{
	return std::filesystem::is_directory(path);
}

bool FileSystem::IsRegular(path_t path)
{
	return std::filesystem::is_regular_file(path);
}

void FileSystem::Rename(path_t path, path_t newName)
{
	std::filesystem::path newPath = path;
	newPath = newPath.replace_filename(newName);
	std::filesystem::rename(path, newPath);
}

void FileSystem::Move(path_t src, path_t dst)
{
	std::rename(src.string().c_str(), dst.string().c_str());
}

void FileSystem::RemoveAll(path_t path)
{
	std::filesystem::remove_all(path);
}

void FileSystem::CreateNewDirectorys(path_t path)
{
	std::filesystem::create_directories(path);
}

void FileSystem::CreateNewDirectory(path_t path)
{
	std::filesystem::path p = path;
	std::string directoryName;
	directoryName = "new_directory";
	p /= directoryName;

	for (int i = 1; i < 64; ++i)
	{
		if (std::filesystem::create_directory(p))
			break;

		auto nextDirectoryName = directoryName;
		nextDirectoryName += std::to_string(i);
		p = path;
		p /= nextDirectoryName;
	}
}

void FileSystem::CreateNewDirectory(path_t path, const char* name)
{
	std::filesystem::path p = path;
	std::string directoryName;
	directoryName = name;
	p /= directoryName;
	auto result = std::filesystem::create_directory(p);
	assert(result);
}

void FileSystem::CreateNewFile(path_t path)
{
	std::ofstream f;
	f.open(path);
	f.close();
}

void FileSystem::Delete(path_t path)
{
	std::filesystem::remove(path);
}

void FileSystem::Copy(path_t src, path_t dst)
{
	std::filesystem::copy(src, dst);
}

std::filesystem::path FileSystem::Absolute(path_t path)
{
	std::filesystem::path p = path;
	if (p.is_relative())
		return std::filesystem::absolute(path);
	return path;
}

std::filesystem::path FileSystem::Relative(path_t path)
{
	std::filesystem::path p = path;
	if (p.is_absolute())
		return p.relative_path();
	return path;
}

std::filesystem::path FileSystem::Proximate(path_t path)
{
	return std::filesystem::proximate(path);
}

std::filesystem::path FileSystem::Extension(path_t path)
{
	std::filesystem::path p{ path };
	return p.extension();
}

std::filesystem::path FileSystem::Stem(path_t path)
{
	std::filesystem::path p{ path };
	return p.stem();
}

std::filesystem::path FileSystem::Parent(path_t path)
{
	std::filesystem::path p{ path };
	return p.parent_path();

}

std::filesystem::path FileSystem::ParentDirectory(path_t path)
{
	std::filesystem::path p{ path };
	p.remove_filename();
	p = p.parent_path();
	return p;
}

std::filesystem::path FileSystem::Current()
{
	return std::filesystem::current_path();
}

std::filesystem::path FileSystem::FileName(path_t path)
{
	return std::filesystem::path(path).filename();
}

std::filesystem::path FileSystem::RemoveFileName(path_t path)
{
	return std::filesystem::path(path).remove_filename();
}

std::filesystem::path FileSystem::InportPath(path_t path)
{
	if (Exists(path) && !IsDirectory(path))
	{
		auto current = Current();
		current.append("");
		std::string p = path.string();
		return p.substr(current.string().size());
	}
	return std::filesystem::path();
}

std::filesystem::path FileSystem::Root(path_t path)
{
	return path.root_path();
}


size_t FileSystem::FileSize(path_t path)
{
	return std::filesystem::file_size(path);
}

FileSystem::FileManager::FileIO FileSystem::FileManager::open(path_t path, FILE_MODE mode)
{
	auto flags = std::ios::in | std::ios::out;
	std::fstream* file = nullptr;
	auto data = FileData();
	auto directory = FileSystem::RemoveFileName(path);

	data.path = path;

	if (directory.has_parent_path() && !Exists(directory.string().c_str()))
		CreateDirectoryPath(directory.string().c_str());
	if (!Exists(path))
		CreateNewFile(path);
	int keyIdx = 0;
	auto n = fileMap.find(keyIdx,data);
	if (n)
	{
		data.file = n->keys[keyIdx].file;
		data.lock = n->keys[keyIdx].lock;
		data.size = n->keys[keyIdx].size;
		data.count = n->keys[keyIdx].count;
		*data.point = 0;
		(*data.count) += 1;
	}
	else
	{
		if (mode == FILE_MODE_BINARY)
			flags |= std::ios::binary;

		data.file = freeList.Get();
		data.lock = lockList.Get();
		data.count = openCountList.Get();
		data.file->open(path, flags);
		data.file->seekg(0, std::ios::end);
		data.size = data.file->tellg();
		data.file->seekg(0, std::ios::beg);
		*data.count = 1;
		fileMap.insert(data);
	}
	assert(*data.count < 100);
	return FileIO(data);
}

void FileSystem::FileManager::close(const FileIO& handle)
{
	assert(handle.fileData.file);

	int keyIdx = 0;;
	auto n = fileMap.find(keyIdx, handle.fileData);
	assert(n);

	(*n->keys[keyIdx].count) -= 1;
	if (*n->keys[keyIdx].count == 0)
	{
		n->keys[keyIdx].file->close();
		freeList.Release(n->keys[keyIdx].file);
		lockList.Release(n->keys[keyIdx].lock);
		openCountList.Release(n->keys[keyIdx].count);
		fileMap.remove(handle.fileData);
	}
}

FileSystem::FileManager::FileIT FileSystem::FileManager::file(path_t directory)
{
	return FileIT(directory);
}

FileSystem::FileManager::DirectoryIT FileSystem::FileManager::directory(path_t path)
{
	return DirectoryIT(path);
}

void FileSystem::FileManager::CreateDirectoryPath(path_t path)
{
	auto directory = std::filesystem::path(path);
	if (!Exists(directory.string().c_str()))
		CreateNewDirectorys(directory.string().c_str());
}


void FileSystem::FileManager::FileIO::read(void* dest, size_type size)const
{
	ScopeLock<SpinLock> l(*fileData.lock);
	assert(fileData.file->is_open());

	fileData.file->seekg(*fileData.point, std::ios::beg);
	fileData.file->read((char*)dest, size);
	*fileData.point = fileData.file->tellg();
	assert(!fileData.file->fail());
}

void FileSystem::FileManager::FileIO::readall(void* dest)const
{
	begin();
	read(dest, fileData.size);
	*fileData.point = fileData.file->tellg();
}

void FileSystem::FileManager::FileIO::read(void* dest, size_type size, size_type offsetInByte)
{
	ScopeLock<SpinLock>l(*fileData.lock);
	if (fileData.file->eof())
		fileData.file->clear();
	fileData.file->seekg(offsetInByte, std::ios::beg);
	fileData.file->read((char*)dest, size);
	*fileData.point = fileData.file->tellg();
	assert(!fileData.file->fail());
}

void FileSystem::FileManager::FileIO::gets(char* dst,size_type buffer)const
{
	fileData.file->seekg(*fileData.point, std::ios::beg);
	fileData.file->getline(dst, buffer, '\0');
	*fileData.point = fileData.file->tellg();
	assert(!fileData.file->fail());
}

void FileSystem::FileManager::FileIO::write(void* src, size_type size)
{
	ScopeLock<SpinLock> l(*fileData.lock);
	assert(fileData.file->is_open());
	fileData.file->seekg(*fileData.point, std::ios::beg);
	fileData.file->write((const char*)src, size);
	*fileData.point = fileData.file->tellg();
	assert(!fileData.file->fail());
}

void FileSystem::FileManager::FileIO::write(void* src, size_type size, size_type offsetInByte)
{
	ScopeLock<SpinLock> l(*fileData.lock);
	assert(fileData.file->is_open());
	fileData.file->seekg(offsetInByte, std::ios::beg);
	fileData.file->write((const char*)src, size);
	*fileData.point = fileData.file->tellg();
	assert(!fileData.file->fail());
}

void FileSystem::FileManager::FileIO::add(void* src, size_type size)
{
	ScopeLock<SpinLock>l(*fileData.lock);
	assert(fileData.file->is_open());
	fileData.file->seekg(0, std::ios::end);
	fileData.file->write((const char*)src, size);
	*fileData.point = fileData.file->tellg();
}

void FileSystem::FileManager::FileIO::begin()const
{
	*fileData.point = 0;
}

void FileSystem::FileManager::FileIO::seek_begin(size_type size)const
{
	*fileData.point = size;
}

void FileSystem::FileManager::FileIO::seek_current(size_type size)const
{
	*fileData.point = distance() + size;
}

size_type FileSystem::FileManager::FileIO::distance()const
{
	return *fileData.point;
}

size_type FileSystem::FileManager::FileIO::size()const
{
	return fileData.size;
}

size_type FileSystem::FileManager::FileIO::lens()const
{
	int len = 0;
	auto start = distance();
	fileData.file->seekg(*fileData.point, std::ios::beg);
	while (!fileData.file->eof() && fileData.file->get() != '\0')
		len++;
	seek_begin(start);
	fileData.file->seekg(*fileData.point, std::ios::beg);
	return len;
}

bool FileSystem::FileManager::FileIO::is_loaded() const
{
	if(fileData.file)
		return fileData.file->is_open();
	return false;
}

FileSystem::path_t FileSystem::FileManager::FileIO::get_path() const
{
	return fileData.path;
}

FileSystem::FileManager::DirectoryIT::path FileSystem::FileManager::DirectoryIT::get()
{
	return iterator->path();
}

bool FileSystem::FileManager::DirectoryIT::last()
{
	return iterator == end;
}

void FileSystem::FileManager::DirectoryIT::next()
{
	iterator++;
	while (iterator != end && !iterator->is_directory())
	{
		iterator++;
	}
}

void FileSystem::FileManager::DirectoryIT::entry()
{
	iterator = begin;
}

FileSystem::FileManager::DirectoryIT::DirectoryIT(path rootDirectory) :
	root(rootDirectory),
	iterator(rootDirectory),
	begin(std::filesystem::begin(iterator)),
	end(std::filesystem::end(iterator))
{
	if (!iterator->is_directory())
	{
		next();
		begin = iterator;
	}
}

FileSystem::FileManager::FileIT::path FileSystem::FileManager::FileIT::get()
{
	return iterator->path();
}

bool FileSystem::FileManager::FileIT::last()
{
	return iterator == end;
}

void FileSystem::FileManager::FileIT::next()
{
	iterator++;
	while (iterator != end && iterator->is_directory())
	{
		iterator++;
	}
}

void FileSystem::FileManager::FileIT::entry()
{
	iterator = begin;
}

FileSystem::FileManager::FileIT::FileIT(path rootDirectory) :
	root(rootDirectory),
	iterator(),
	begin(std::filesystem::begin(iterator)),
	end(std::filesystem::end(iterator))
{
	if (Exists(rootDirectory))
		iterator = std::filesystem::directory_iterator(rootDirectory);
}

bool FileSystem::FileManager::FileData::operator<(const FileData&o) const
{
	return path < o.path;
}

bool FileSystem::FileManager::FileData::operator<=(const FileData&o) const
{
	return path <= o.path;
}

bool FileSystem::FileManager::FileData::operator>(const FileData& o) const
{
	return path > o.path;
}

bool FileSystem::FileManager::FileData::operator>=(const FileData& o) const
{
	return path >= o.path;
}

bool FileSystem::FileManager::FileData::operator==(const FileData& o) const
{
	return path == o.path;
}

void FileSystem::FileManager::FileData::operator=(const FileData& o)
{
	path = o.path;
	file = o.file;
	size = o.size;
	lock = o.lock;
	count = o.count;
	point = new size_type;
	*point = *o.point;
}

void FileSystem::FileManager::FileData::operator=(FileData&& o)
{
	path = o.path;
	file = o.file;
	size = o.size;
	lock = o.lock;
	point = o.point;
	count = o.count;

	o.path = "";
	o.file = nullptr;
	o.size = 0;
	o.lock = nullptr;
	o.point = nullptr;
	o.count = nullptr;
}

FileSystem::FileManager::FileData::FileData(const FileData& o)
{
	*this = o;
}

FileSystem::FileManager::FileData::FileData(FileData&& o)
{
	*this = std::move(o);
}
