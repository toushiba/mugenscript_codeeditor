#pragma once
#include <inttypes.h>
#include <atomic>
#include "thread/coroutine.h"
#include "Container.h"
#include "UniqueID.h"
#include "Alloc.h"
#define FINISHIED_JOB 1

namespace Job
{
	class Counter;
	class Observer;
	typedef Coroutine::generator<int> generator_t;
	typedef uintptr_t argment_t;
	typedef generator_t EntryPoint(argment_t);
	typedef typename Container::AsyncFreeListEx<Counter>::data_t pCounter_t;
	typedef typename Container::AsyncFreeListEx<Observer>::data_t pObserver_t;

	typedef void CallbackFunction();
	enum class Priority
	{
		LOW,
		NORMAL,
		HIGH,
		CRITICAL
	};

	enum class State
	{
		NONE,
		JOIN,
		DETOUCH,
	};

	enum class JobState
	{
		NONE,
		WAIT,
		RUN,
		FINISH
	};

	class Counter
	{
		std::atomic_int count;
	public:
		void Take();
		void Release();
		int GetCount()const;
		Counter() :count() {};
		Counter(const Counter&) = delete;
		void operator=(const Counter&) = delete;
	};

	class Observer
	{
	public:
		std::atomic<State> state;
		int value;
		int num;
	};

	struct Declaration
	{
		struct Option
		{
			Priority priority;
			pCounter_t counter;
			pObserver_t observer;
			generator_t* generator;
			CallbackFunction* callback;
			Option() :priority(), counter(), generator(), callback(),observer() {}
		};

		EntryPoint* entryPoint;
		uintptr_t param;
		Option option;
		Declaration() :entryPoint(), param(), option() {}
	};

	void InitJobSystem();
	void EndJobSystem();
	void KickJob(Declaration& decl);
	void KickJobs(int count, Declaration decl[]);
	void KickJobAndWait(Declaration& decl);
	void KickJobsAndWait(int count, Declaration decl[]);
	void KickJobAndDetouch(Declaration& decl);
	void KickJobsAndDetouch(int count, Declaration decl[]);
	void JoinJob( Declaration&);
	void JoinJobs(int count, Declaration decl[]);
	void DetouchJob(Declaration& decl);
	void DetouchJobs(int count, Declaration decl[]);
	void JobWorkerThread();
	Job::pCounter_t AllocCounter();
	void DeallocCounter(Job::pCounter_t);
	JobState CheckJobState(Declaration& decl);
}

