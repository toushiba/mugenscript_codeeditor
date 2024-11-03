#pragma once
#include <experimental/coroutine>

namespace Coroutine
{
	template<class T>
	class generator
	{
	public:
		struct promise_type;
		using handle = std::experimental::coroutine_handle<promise_type>;
		struct promise_type
		{
			T result;
			auto get_return_object() { return generator{ handle::from_promise(*this) }; }
			auto initial_suspend() { return std::experimental::suspend_always{}; }
			auto final_suspend()noexcept { return std::experimental::suspend_always{}; }
			void unhandled_exception() { std::terminate(); }
			void return_void() {}
			auto yield_value(T value)
			{
				result = value;
				return std::experimental::suspend_always{};
			}
		};

		bool move_next() { return coro ? (coro.resume(), !coro.done()) : false; }//最終サスペンドポイントならfalseを返す
		bool handle_state()const { return (bool)coro; }
		T current_value() { return coro.promise().result; }
		bool done() { return coro.done(); }
		void init() { coro = handle(); }
		void operator=(generator const&) = delete;
		generator(generator const&) = delete;
		generator(generator&& rhs)noexcept :coro(rhs.coro) { rhs.coro = nullptr; };
		~generator() { if (coro)coro.destroy(); }
	private:
		handle coro;
		generator(handle h):coro(h){}
	};
}