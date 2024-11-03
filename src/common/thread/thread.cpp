#include "common/thread/thread.h"
#include "common/thread/spinlock.h"
#include <vector>
#include <queue>

struct ThreadRegister
{
	SpinLock queueLock;

	std::vector<MyThread::thread_t> threadList;
}static threadRegister;

MyThread::thread_t&& ModeThread(MyThread::tid);

void MyThread::PushThread(thread_t&& thread)
{
	ScopeLock<SpinLock> scopeLock(threadRegister.queueLock);
	threadRegister.threadList.push_back(std::move(thread));
}

MyThread::thread_t* MyThread::GetThread(tid id)
{
	ScopeLock<SpinLock> scopeLock(threadRegister.queueLock);
	for (auto& a : threadRegister.threadList)
		if (a.get_id() == id)
			return &a;
	return nullptr;
}

bool MyThread::Joinable(tid id)
{
	if (id == tid())
		return false;

	auto t = GetThread(id);

	if (t)
		return t->joinable();
	return false;
}

void MyThread::Detach(tid id)
{
	auto t=GetThread(id);
	if (t&&t->joinable())
	{
		t->detach();
	}
}

void MyThread::Join(tid id)
{
	auto t = GetThread(id);
	if (t&&t->joinable())
	{
		t->join();
	}
}

void MyThread::CleanUpThreads()
{
	ScopeLock<SpinLock> queueLock(threadRegister.queueLock);
	size_t count = 0;
	for (auto& a : threadRegister.threadList)
	{
		if (a.get_id() == tid())
		{
			if (threadRegister.threadList.size() > 1)
				a.swap(threadRegister.threadList.back());
			count++;
		}
	}

	for (size_t i = 0; i < count; ++i)
		threadRegister.threadList.pop_back();

}
