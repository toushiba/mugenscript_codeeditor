#include "common/thread/spinlock.h"

bool SpinLock::TryAquire()const
{
	bool alreadyLocked = atomic->test_and_set(std::memory_order_acquire);
	return !alreadyLocked;
}

void SpinLock::Aquire()const
{
	while (!TryAquire())
		std::this_thread::yield();
}

void SpinLock::Release()const
{
	atomic->clear(std::memory_order_release);	
}

void ReentrantLock32::Aquire()
{
	std::hash<std::thread::id> hasher;
	std::size_t tid = hasher(std::this_thread::get_id());

	//現在のスレッドがロックを保持していないなら
	if (atomic.load(std::memory_order_relaxed) != tid)
	{
		//ロックを掴むまで待機
		std::size_t unlockValue = 0;
		while (!atomic.compare_exchange_weak(
			unlockValue,
			tid,
			std::memory_order_relaxed,
			std::memory_order_relaxed))
		{
			unlockValue = 0;
			std::this_thread::yield();
		}
	}
	++refCount;
	std::atomic_thread_fence(std::memory_order_acquire);
}

void ReentrantLock32::Release()
{
	std::atomic_thread_fence(std::memory_order_release);

	std::hash<std::thread::id> hasher;
	std::size_t tid = hasher(std::this_thread::get_id());
	std::size_t actual = atomic.load(std::memory_order_relaxed);
	assert(actual == tid);

	--refCount;
	if (refCount == 0)
		atomic.store(0, std::memory_order_relaxed);
}

bool ReentrantLock32::TryAquire()
{
	std::hash<std::thread::id> hasher;
	std::size_t tid = hasher(std::this_thread::get_id());

	bool acquired = false;

	if (atomic.load(std::memory_order_relaxed) == tid)
		acquired = true;
	else
	{
		std::size_t unlockValue = 0;
		acquired = atomic.compare_exchange_strong(
			unlockValue,
			tid,
			std::memory_order_relaxed,
			std::memory_order_relaxed
		);
	}

	if (acquired)
	{
		refCount++;
		std::atomic_thread_fence(std::memory_order_acquire);
	}
	return acquired;
}

void UnnecessaryLock::Aquire()
{
	//誰もロックを持っていないと仮定
	assert(!locked);
	//ここでロック(クリティカル操作のオーバーラップが発生しても検出できる)
	locked = true;
}

void UnnecessaryLock::Release()
{
	//正しい使い方が成されていると仮定
	assert(locked);

	//アンロック
	locked = false;
}

bool ReaderWriterLock::Reader::TryAquier()const
{
	counter_t current = pLock->counter.load();
	return pLock->counter& (1u << writerBit) ? 
		false : 
		pLock->counter.compare_exchange_weak(current, current + 1);
}

bool ReaderWriterLock::Reader::TryRelease()const
{
	counter_t current = pLock->counter.load();
	return pLock->counter.compare_exchange_weak(current, current - 1);
}

void ReaderWriterLock::Reader::Aquire()const
{
	while (!TryAquier())
	{
		std::this_thread::yield();
	}
}

void ReaderWriterLock::Reader::Release()const
{
	while (!TryRelease())
	{
		std::this_thread::yield();
	}
}

bool ReaderWriterLock::Writer::TryAquier()const
{
	counter_t current = pLock->counter.load();

	return (pLock->counter & (1u << writerBit)) ?
		pLock->counter == (1u << writerBit) :
		pLock->counter.compare_exchange_weak(current, current | (1u << writerBit)) && pLock->counter ==writerBit;
}

bool ReaderWriterLock::Writer::TryRelease()const
{
	counter_t current = pLock->counter.load();
	return pLock->counter.compare_exchange_weak(current, current & ~(1 << writerBit));
}

void ReaderWriterLock::Writer::Aquire()const
{
	spinLock.Aquire();

	while(!TryAquier())
		std::this_thread::yield();	
}

void ReaderWriterLock::Writer::Release()const
{
	while(!TryRelease())
		std::this_thread::yield();
	spinLock.Release();
}

bool UniqueLock::Aquire() const
{
	return !lock->test_and_set(std::memory_order_acquire);
}

void UniqueLock::Release() const
{
	lock->clear(std::memory_order_release);
}

UniqueLock::SharedLock UniqueLock::Shared() const
{
	return SharedLock(this);
}

const bool UniqueLock::SharedLock::Locked()const
{
	return lock->_Storage;
}

void UniqueLock::SharedLock::Wait() const
{
	while (lock->_Storage)
	{
		std::this_thread::yield();
	}
}

UniqueLock::SharedLock::SharedLock(const UniqueLock* l):lock(l->lock)
{
}
