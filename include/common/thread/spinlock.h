#pragma once
#include <atomic>
#include <thread>
#include <assert.h>
#include "spinlock.h"

class SpinLock
{
	std::atomic_flag* atomic;

public:

	SpinLock() :atomic(new std::atomic_flag) {};
	~SpinLock() { delete atomic; }
	
	bool TryAquire()const;
	void Aquire()const;
	void Release()const;
};


class ReentrantLock32
{
	std::atomic<std::size_t> atomic;
	std::uint32_t refCount;

public:
	ReentrantLock32() :atomic(0), refCount(0) {}
	void Aquire();
	void Release();
	bool TryAquire();
};

class UnnecessaryLock
{
	volatile bool locked;

public:

	void Aquire();
	void Release();
};

class UnnecessaryLockJanitor
{
	UnnecessaryLock* pLock;
public:

	explicit UnnecessaryLockJanitor(UnnecessaryLock& queueLock) :pLock(&queueLock) { pLock->Aquire(); }
	~UnnecessaryLockJanitor() { pLock->Release(); }
};

template<class Lock>
class ScopeLock
{
	typedef Lock lock_t;
	const lock_t* pLock;

public:

	explicit ScopeLock(const lock_t& queueLock) :pLock(&queueLock)
	{
		queueLock.Aquire();
	}

	~ScopeLock()
	{
		pLock->Release();
	}
};

template<class T>
class SList
{
	struct Node
	{
		T data;
		Node* next;
	};
	std::atomic<Node*> head{ nullptr };

public:

	void push_front(T data)
	{
		auto newNode = new Node();
		newNode->data = data;
		newNode->next = head;

		while (!head.compare_exchange_weak(newNode->next, newNode))
		{
		}
	}
};

class ReaderWriterLock
{
	using counter_t = unsigned int;
	std::atomic<counter_t> counter;
	static const unsigned char writerBit = sizeof(counter_t) * 8 - 1;
public:

	class Reader
	{
		ReaderWriterLock* pLock;

	public:
		bool TryAquier()const;
		bool TryRelease()const;
		void Aquire()const;
		void Release()const;
		Reader(ReaderWriterLock* lock) :pLock(lock) {};
		~Reader(){}
	};

	class Writer
	{
		SpinLock spinLock;
		ReaderWriterLock* pLock;

	public:
		bool TryAquier()const;
		bool TryRelease()const;
		void Aquire()const;
		void Release()const;
		Writer(ReaderWriterLock* lock) :pLock(lock) {};
		~Writer(){}
	};

	Reader reader;
	Writer writer;
	void clear() { counter = 0; }
	ReaderWriterLock() :reader(this), writer(this) {};
	~ReaderWriterLock() {};
};

class UniqueLock
{
public:

	class SharedLock
	{
	public:

		const bool Locked()const;
		void Wait()const;
		SharedLock(const UniqueLock* l);
	private:
		std::atomic_flag* lock;
	};

	bool Aquire()const;
	void Release()const;
	SharedLock Shared()const;

	UniqueLock() :lock(new std::atomic_flag) {};
	~UniqueLock() { delete lock; }

private:

	std::atomic_flag* lock;
};