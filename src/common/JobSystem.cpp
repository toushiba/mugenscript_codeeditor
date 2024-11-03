#include "common/JobSystem.h"
#include "common/thread/spinlock.h"
#include "common/thread/Mutex.h"
#include "common/thread/thread.h"
#include "common/Container.h"
#include <queue>
#include <list>
#define COUNTER_RESERVE_SIZE 256

struct JobManager
{
	bool isActive;
	std::vector<MyThread::tid> threadIds;
	Container::AsyncQueue<Job::Declaration> jobQueue;
	SpinLock coroutineLock;
	std::list<Job::generator_t> coroutines;
	Container::AsyncFreeListEx<Job::Counter> counterFreelist;
	Container::AsyncFreeListEx<Job::Observer> observerFreelist;
}static jobManager;

void PushJobQueue(const Job::Declaration& job);
bool PopJobQueue(Job::Declaration);
void PushGenerator(Job::generator_t&&);
void RemoveGenerator(Job::generator_t*);
bool IsReady();

Job::pObserver_t AllocObserver();
void DeallocObserver(Job::pObserver_t);




Job::pObserver_t AllocObserver()
{
	auto observer = jobManager.observerFreelist.Get();
	observer->state = Job::State::NONE;
	observer->value = 0;
	observer->num = 0;
	return observer;
}

void DeallocObserver(Job::pObserver_t a)
{
	jobManager.observerFreelist.Release(a);
}

void Job::InitJobSystem()
{
	auto threadNum = std::thread::hardware_concurrency();
	jobManager.isActive = true;
	for (auto i = 0u; i < threadNum; ++i)
	{
		auto id = MyThread::CreateThread(&Job::JobWorkerThread);
		jobManager.threadIds.push_back(id);
	}
}

void Job::EndJobSystem()
{
	jobManager.isActive = false;

	for (auto& a : jobManager.threadIds)
	{
		MyThread::Join(a);
	}
}

void Job::KickJob(Declaration& decl)
{
	assert(jobManager.isActive);

	auto g = decl.entryPoint(decl.param);
	decl.option.observer = AllocObserver();
	//ロックを取得
	jobManager.coroutineLock.Aquire();
	jobManager.coroutines.push_back(std::move(g));
	decl.option.generator = &jobManager.coroutines.back();
	//ロック解除
	jobManager.coroutineLock.Release();

	if (decl.option.counter)
		decl.option.counter->Take();
	PushJobQueue(decl);
}

void Job::KickJobs(int count, Declaration decl[])
{
	for (int i = 0; i < count; ++i)
	{
		KickJob(decl[i]);
	}
}

void Job::KickJobAndWait(Declaration& decl)
{
	KickJob(decl);
	JoinJob(decl);
}

void Job::KickJobsAndWait(int count, Declaration decl[])
{
	KickJobs(count, decl);
	JoinJobs(count, decl);
}

void Job::KickJobAndDetouch(Declaration& decl)
{
	KickJob(decl);
	DetouchJob(decl);
}

void Job::KickJobsAndDetouch(int count, Declaration decl[])
{
	KickJobs(count, decl);
	DetouchJobs(count, decl);
}

void Job::JoinJob(Declaration& decl)
{
	auto current = decl.option.observer->state.load();
	if (current == Job::State::NONE && decl.option.observer->state.compare_exchange_weak(current, Job::State::JOIN))
	{
		while (decl.option.generator->handle_state())
		{
			std::this_thread::yield();
		}

		RemoveGenerator(decl.option.generator);
		DeallocObserver(decl.option.observer);
		
	}
}

void Job::JoinJobs(int count, Declaration decl[])
{
	for (int i = 0; i < count; ++i)
		JoinJob(decl[i]);
}

void Job::DetouchJob(Declaration& decl)
{
	auto current = decl.option.observer->state.load();
	
	if (current == Job::State::NONE && decl.option.observer->state.compare_exchange_weak(current, Job::State::DETOUCH))
	{
		if (!decl.option.generator->handle_state())
		{
			RemoveGenerator(decl.option.generator);
			DeallocObserver(decl.option.observer);
		}

	}
}

void Job::DetouchJobs(int count, Declaration decl[])
{
	for (int i = 0; i < count; ++i)
		DetouchJob(decl[i]);
}


void Job::JobWorkerThread()
{
	while (jobManager.isActive)
	{
		Declaration declcpy;
		//queueが空の時にストップ
		while (!IsReady())
		{
			std::this_thread::yield();

			if (!jobManager.isActive)
				return;
		}

		if (!PopJobQueue(declcpy))
			continue;

		auto* g = declcpy.option.generator;

		if (g && g->move_next())
			PushJobQueue(declcpy);//コルーチンが終了していないならジョブをもう一度プッシュ
		else
		{
			//コルーチンが終了した場合の処理
			if (declcpy.option.counter)
			{
				//カウンターの解放
				declcpy.option.counter->Release();
				//ジョブの終了時に呼び出す関数

				//カウンターが0ならコールバック呼び出し
				if (declcpy.option.counter->GetCount() == 0)
				{
					if(declcpy.option.callback)
						declcpy.option.callback();
					DeallocCounter(declcpy.option.counter);
				}
			}
			else
			{
				if (declcpy.option.callback)
					declcpy.option.callback();
			}

			if (declcpy.option.observer->state == Job::State::DETOUCH)
			{
				RemoveGenerator(g);
				DeallocObserver(declcpy.option.observer);
			}
		}

	}
}

void PushJobQueue(const Job::Declaration& job)
{
	jobManager.jobQueue.push(job);
}

bool PopJobQueue(Job::Declaration decl)
{
	return jobManager.jobQueue.pop_front(decl);
}

void PushGenerator(Job::generator_t&& g)
{
	ScopeLock<SpinLock> lock(jobManager.coroutineLock);
	jobManager.coroutines.push_back(std::move(g));
}

void RemoveGenerator(Job::generator_t* g)
{
	if (!g)
		return;

	ScopeLock<SpinLock> lock(jobManager.coroutineLock);
	for (auto it = jobManager.coroutines.begin(); it != jobManager.coroutines.end();)
		if (&(*it) == g)
		{
			it = jobManager.coroutines.erase(it);
		}
		else
			++it;
}

bool IsReady()
{
	return jobManager.jobQueue.empty();
}

void Job::Counter::Take()
{
	count++;
}

void Job::Counter::Release()
{
	if (0 < count)
		count--;
}

int Job::Counter::GetCount()const
{
	return count.load();
}

Job::pCounter_t Job::AllocCounter()
{
	return jobManager.counterFreelist.Get();
}

void Job::DeallocCounter(Job::pCounter_t counter)
{
	jobManager.counterFreelist.Release(counter);
}

Job::JobState Job::CheckJobState(Declaration& decl)
{
	if (!decl.option.generator)
		return JobState::NONE;
	if (decl.option.generator->done())
	{
		return JobState::RUN;
	}
	else
	{
		return JobState::FINISH;
	}
}
