#pragma once
#include <thread>

namespace MyThread
{
	typedef typename std::thread::id tid;
	typedef std::thread thread_t;

	template<class T, class...U>
	tid CreateThread(T(*func)(U...), U... arg);
	template<class T,class W, class...U>
	tid CreateThread(T(W::* func)(U...), W* obj, U... arg);
	void PushThread(thread_t&&);
	void Detach(tid);
	void Join(tid);
	bool Joinable(tid);
	thread_t* GetThread(tid);
	void CleanUpThreads();

	template<class T,class...U>
	class Thread;

	template<class T,class ...U>
	class Thread<T(U...)>
	{
		typedef T(*FuncType)(U...);
		typedef Thread<T(U...)> ThisType;
		tid id;
		FuncType function;
		T rValue;
		bool end;
		void Function(U...arg) 
		{
			auto r = function(arg...);
			rValue = r;
			end = true;
		};
	public:
		bool EndThread() { return end; };
		T result() { return rValue; }
		operator tid() { return id; }
		Thread(T(*func)(U...), U...arg) :function(func),id(CreateThread<void,ThisType,U...>(&ThisType::Function,this, arg...)),end(false) {};
		Thread(const Thread<T, U...>&) = delete;
		void operator = (const Thread<T, U...>&) = delete;
		~Thread() { if (Joinable(id)) Join(id); }
	};
	
	template<class ...U>
	class Thread<void(U...)>
	{
		typedef void(*FuncType)(U...);
		typedef Thread<void(U...)> ThisType;
		tid id;
		FuncType function;
		bool end;
		void Function(U...arg) { function(arg...); end = true; };
	public:
		bool EndThread() { return end; }
		Thread(void(*func)(U...), U...arg) :function(func), id(CreateThread<void,ThisType,U...>(&ThisType::Function,this, arg...)),end(false) {};
		~Thread() { if (Joinable(id)) Join(id); }
	};
}

template<class T, class ...U>
MyThread::tid MyThread::CreateThread(T(*func)(U...), U ...arg)
{
	thread_t th(func, arg...);
	tid id = th.get_id();
	PushThread(std::move(th));
	return id;
}

template<class T, class W, class ...U>
MyThread::tid MyThread::CreateThread(T(W::*func)(U...),W* obj, U ...arg)
{
	thread_t th(func, obj, arg...);
	tid id = th.get_id();
	PushThread(std::move(th));
	return id;
}