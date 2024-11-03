#pragma once
#include <utility>
#include <cassert>
#include <atomic>
#include <memory.h>
#include "thread/spinlock.h"
#include "type.h"
#define STACK_BLOCK_SIZE sizeof(char)


static constexpr size_type nbitAlign = 5;
static constexpr size_type nAlign = 1 << nbitAlign;


class StackAllocator
{
	typedef uintptr_t Marker;
	size_type byteSize;
	Marker marker;
	void* stack;
public:
	//バイトサイズを確保
	explicit StackAllocator(size_type stackSizeByte);
	//メモリを割り当てる
	void* alloc(size_type sizeByte);


	//スタックの先頭位置を返す
	Marker GetMarker();
	//先頭位置を以前のマーカーにロールバック
	void FreeToMarker(Marker marker);
	//スタック全体をクリア
	void clear();
	~StackAllocator();
};

template<class T>
struct PoolAllocator
{
	union PoolData
	{
		T data;
		size_type next;
		PoolData():data(){}
		~PoolData(){}
	};

	PoolData* head;
	size_type nextIdx;
	size_t blockSize;
	size_t counter;
	void DeallocPoolData(PoolData* data);
public:

	explicit PoolAllocator(size_type blockSize);
	T* alloc();
	void dealloc(T*);
	void clear();
	size_t count();
	~PoolAllocator();
};



class DoubleBufferAllocator
{
	uint32_t curStack;
	StackAllocator stack[2];
public:

	void SwapBuffer();
	void ClearCurrentBuffer();
	void* alloc(uint32_t byteSize);
};

template<class T>
class AsyncAllocatorReader
{
public:

	using this_t = AsyncAllocatorReader<T>;

	operator T* ()const { return p; };
	operator T& ()const { return *p; };
	T* operator->()const { return p; };
	T& operator*()const { return *p; };
	void operator=(const T& v) { *p = v; };
	void operator=(T&& v) { *p = std::move(v); }
	void operator=(const this_t&) = delete;
	void operator=(this_t&&) = delete;

	AsyncAllocatorReader(const ReaderWriterLock* lock, T* p) :lock(lock), p(p) { if (lock)lock->reader.Aquire(); };
	AsyncAllocatorReader(const this_t&) = delete;
	AsyncAllocatorReader(this_t&&) = delete;
	~AsyncAllocatorReader() { if (lock)lock->reader.Release(); }

private:

	const ReaderWriterLock* lock;
	T* p;
};

template<class allocator_t,class access_type>
class AsyncAllocatorRLockData
{
public:

	using this_t = AsyncAllocatorRLockData<allocator_t, access_type>;
	using Reader = typename allocator_t::Reader;
	using access_t = access_type;

	Reader operator->()const { return Reader(lock, allocator_t::Access(a,v)); };
	Reader operator*()const { return Reader(lock, allocator_t::Access(a, v)); };
	Reader operator[](access_t v)const { return Reader(lock, allocator_t::Access(a, v)); }
	operator Reader()const { return Reader(lock, allocator_t::Access(a, v)); }
	operator bool()const { return a; };
	void operator=(const this_t& o) { a = o.a; lock = o.lock; v = o.v; };
	bool operator==(const this_t& o)const { return v == o.v; };
	access_t AccessValue() { return v; }
	AsyncAllocatorRLockData() :a(), lock(), v(){}
	AsyncAllocatorRLockData(allocator_t* a, ReaderWriterLock* lock, typename allocator_t::value_type* ptr) :a(a), lock(lock), v(allocator_t::Index(ptr)) {};
	AsyncAllocatorRLockData(allocator_t* a,ReaderWriterLock* lock, access_t v) :a(a),lock(lock), v(v) {};
	AsyncAllocatorRLockData(const AsyncAllocatorRLockData&) = default;
	~AsyncAllocatorRLockData() = default;

private:

	void operator=(typename allocator_t::value_type* ptr) { v = allocator_t::Index(a, ptr); }
	
	friend allocator_t;
	allocator_t* a;
	ReaderWriterLock* lock;
	access_t v;
};

template<class T>
class AsyncPoolAllocator
{

	union PoolData
	{
		T data;
		size_type next;
		PoolData():data(){}
		~PoolData(){}
	};

	void MemResize(const size_type preSize, const size_type newSize);

public:

	using value_type = T;
	using pointer_t = std::add_pointer_t<T>;
	using this_t = AsyncPoolAllocator<T>;
	using Reader = AsyncAllocatorReader<T>;
	using data_t = AsyncAllocatorRLockData<this_t, size_type>;

	this_t::data_t alloc();
	void dealloc(this_t::data_t);
	void dealloc_obj(T*);
	void clear();

	template<class...Args>
	void construct(data_t data, Args...args);
	void destruct(data_t);
	
	static T* Access(this_t* a,size_type value);
	static size_type Index(this_t* a, T* ptr);
	
	AsyncPoolAllocator() :head(), nextIdx(-1), align(nAlign)
	{
		MemResize(0, nAlign);
	}

	AsyncPoolAllocator(size_type size) :head(), nextIdx(0), align(nAlign)
	{
		MemResize(0, (size + nAlign) & ~(nAlign - 1));
	}

	AsyncPoolAllocator(size_type size, size_type align) :head(), nextIdx(0), align(align)
	{
		MemResize(0, (size + align) & ~(align - 1));
	}


private:

	std::atomic<PoolData*> head;
	std::atomic<size_type> nextIdx;
	std::atomic<size_type> blockSize;
	ReaderWriterLock lock;
	size_type align;
};

template<class T>
class AsyncArrayAllocator
{
public:

	using this_t = AsyncArrayAllocator<T>;
	using data_t = AsyncAllocatorRLockData<T,size_type>;
	using Reader = AsyncAllocatorReader<T>;

	data_t alloc(size_type);
	data_t realloc(size_type);
	void dealloc(data_t);

	T* Access(this_t* a, size_type v);

private:

	T* head;
	ReaderWriterLock lock;
	size_type capacity;
	size_type size;
};

template<class T>
class DefaultAllocator
{
public:

	using this_t = DefaultAllocator<T>;
	using data_t = T*;

	data_t alloc(size_type size);
	void dealloc(data_t ptr);

};

template<class T>
class MemAllocator
{
public:

	using this_t = MemAllocator<T>;
	using data_t = T*;

	data_t alloc(size_type size);
	void dealloc(data_t ptr);
};

inline uintptr_t AlignAddress(uintptr_t addr, size_t align)
{
	const size_t mask = align - 1;
	assert((align & mask) == 0);
	return (addr + mask) & ~mask;
}

template<class T>
inline T* AlignPointer(T* ptr, size_t align)
{
	const uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
	const uintptr_t addrAligned = AlignAddress(addr, align);
	return reinterpret_cast<T*>(addrAligned);
}

void* AllocAligned(size_t bytes, size_t align);
void FreeAligned(void* pMem);

template<class T>
inline PoolAllocator<T>::PoolAllocator(size_type blockSize):
	head(new PoolData[blockSize]{}), nextIdx(), blockSize(blockSize), counter()
{
	for (size_type i = 0; i < blockSize; ++i)
		head[i].next = i + 1;
	head[blockSize - 1].next = -1;
}

template<class T>
inline T* PoolAllocator<T>::alloc()
{
	if (counter == blockSize)
		return nullptr;
	const size_type idx = nextIdx;
	nextIdx = head[nextIdx].next;
	++counter;

	head[idx].next = 0;
	return &head[idx].data;
}

template<class T>
inline void PoolAllocator<T>::DeallocPoolData(PoolData* data)
{
	const size_type idx = (size_type)((data - head) / sizeof(PoolData));
	
	assert(0 <= idx && idx < blockSize&& counter>0);
	head[idx].next = nextIdx;
	--counter;
	nextIdx = idx;
}

template<class T>
inline void PoolAllocator<T>::dealloc(T* p)
{
	DeallocPoolData((PoolData*)p);
}

template<class T>
inline void PoolAllocator<T>::clear()
{
	for (size_t i = 0; i < blockSize; ++i)
		head[i].next = i + 1;
	head[blockSize - 1].next = -1;
}

template<class T>
inline size_t PoolAllocator<T>::count()
{
	return counter;
}

template<class T>
inline PoolAllocator<T>::~PoolAllocator()
{
	delete[] head;
}

namespace Allocator
{
	class ResDxAllocTag
	{
	public:

		~ResDxAllocTag() = default;
		ResDxAllocTag(uint16_t size);
	};
}

template<class T>
inline void AsyncPoolAllocator<T>::MemResize(const size_type preSize,const size_type size)
{
	assert(size);

	PoolData* newPtr = new PoolData[size]();
	PoolData* prePtr = head.load();
	
	ScopeLock<ReaderWriterLock::Writer> lock(this->lock.writer);
	
	if (head)
	{
		memcpy(newPtr, head, preSize * sizeof(PoolData));
	}

	for (size_type i = preSize; i < size; ++i)
	{
		newPtr[i].next = i + 1;
	}

	newPtr[size - 1].next = -1;
	blockSize = size;
	head.store(newPtr);
	nextIdx = preSize;

	if (prePtr)
		delete[] prePtr;

}

template<class T>
inline typename AsyncPoolAllocator<T>::data_t AsyncPoolAllocator<T>::alloc()
{
	auto next = nextIdx.load();

	while(next == -1||!nextIdx.compare_exchange_weak(next, head[next].next))
	{
		std::this_thread::yield();
		next = nextIdx.load();
	}

	size_type preSize = blockSize;

	//次のリストが無い場合
	if ((nextIdx == -1 || !head) && 
		(blockSize != 0) && blockSize.compare_exchange_weak(preSize,0))
	{
		MemResize(preSize,(preSize + align) & ~(align - 1));
	}

	return this_t::data_t(this,&lock,next);
}

template<class T>
inline void AsyncPoolAllocator<T>::dealloc(this_t::data_t data)
{
	size_type index = data.v;
	PoolData* pData = &head[index];
	auto next = nextIdx.load();
	pData->next = next;

	while (!nextIdx.compare_exchange_weak(next, index))
	{
		std::this_thread::yield();
		next = nextIdx.load();
		pData = &head[index];
		pData->next = next;		
	}

}

template<class T>
inline void AsyncPoolAllocator<T>::dealloc_obj(T* p)
{
	assert(head < p&& p < head + blockSize * sizeof(PoolData));
	dealloc(data_t(this, &lock, ((uintptr_t)p - (uintptr_t)head) / sizeof(PoolData)));
}



template<class T>
inline void AsyncPoolAllocator<T>::clear()
{

}

template<class T>
inline void AsyncPoolAllocator<T>::destruct(data_t data)
{
	ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
	head[data.v].data.~T();
}

template<class T>
inline T* AsyncPoolAllocator<T>::Access(this_t* a, size_type value)
{
	return &a->head[value].data;
}

template<class T>
inline size_type AsyncPoolAllocator<T>::Index(this_t* a, T* ptr)
{
	return ((uintptr_t)ptr - (uintptr_t)a->head.load()) / sizeof(PoolData);
}

template<class T>
template<class ...Args>
inline void AsyncPoolAllocator<T>::construct(data_t data, Args ...args)
{
	ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
	::new(&head[data.v])T(args...);
}

template<class T>
inline typename AsyncArrayAllocator<T>::data_t AsyncArrayAllocator<T>::alloc(size_type size)
{
	auto newPtr = new T[size];
	ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
	head = newPtr;
	capacity = size;
	return data_t(this, &lock, 0);
}

template<class T>
inline typename AsyncArrayAllocator<T>::data_t AsyncArrayAllocator<T>::realloc(size_type size)
{
	auto newPtr = new T[size];
	auto prePtr = head;

	ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
	memcpy(newPtr, head);
	head = newPtr;
	delete prePtr;

	return data_t(this, &lock, 0);
}

template<class T>
inline void AsyncArrayAllocator<T>::dealloc(AsyncArrayAllocator<T>::data_t p)
{
	lock.writer.Aquire();
	delete[] head;

	lock.writer.Release();
}

template<class T>
inline T* AsyncArrayAllocator<T>::Access(this_t* a, size_type v)
{
	return head[v];
}

template<class T>
inline typename DefaultAllocator<T>::data_t DefaultAllocator<T>::alloc(size_type size)
{
	T* newPtr= ::new T[size]();
	return newPtr;
}

template<class T>
inline void DefaultAllocator<T>::dealloc(data_t ptr)
{
	delete[] ptr;
}

template<class T>
inline typename MemAllocator<T>::data_t MemAllocator<T>::alloc(size_type size)
{
	return (data_t)malloc(size * sizeof(T));
}
template<class T>
inline void MemAllocator<T>::dealloc(data_t ptr)
{
	free((void*)ptr);
}
