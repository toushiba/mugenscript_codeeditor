#pragma once
#include <mutex>

class Mutex
{
	bool isLocked;
	std::mutex queueLock;
public:

	void Lock();
	bool TryLock();
	void Unlock();
	Mutex():queueLock(),isLocked(false) {};
	~Mutex() { Unlock(); }
};