#include "common/thread/Mutex.h"

void Mutex::Lock()
{
	if (!isLocked)
	{
		queueLock.lock();
		isLocked = true;
	}
}

bool Mutex::TryLock()
{
	return queueLock.try_lock();
}

void Mutex::Unlock()
{
	if (isLocked)
	{
		queueLock.unlock();
		isLocked = false;
	}
}

