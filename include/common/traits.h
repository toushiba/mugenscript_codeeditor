#pragma once
#include <utility>
#include <cassert>

namespace Traits
{
	//--------------------------copy_traits-----------------------------------------------------------------

	template<bool,class T>
	struct if_copy_traits;
	template<bool, class T>
	struct if_move_traits;

	template<class T>
	struct if_copy_traits<false, T>
	{
		using reference_t = typename std::add_lvalue_reference<T>::type;
		using pointer_t = T*;
		static void copy(reference_t dst, const reference_t src) { assert(false); };
		static void copy_n(pointer_t dst, const pointer_t src, size_type n) { assert(false); }
	};

	template<class T>
	struct if_move_traits<false, T>
	{
		using lmove_t = typename std::add_lvalue_reference<T>::type;
		using rmove_t = typename std::add_rvalue_reference<T>::type;
		static void move(lmove_t l, rmove_t r) { assert(false); }
	};

	template<class T>
	struct if_copy_traits<true,T>
	{
		using reference_t = typename std::add_lvalue_reference<T>::type;
		using pointer_t = T*;
		static void copy(reference_t dst, const reference_t src) { dst = src; };
		static void copy_n(pointer_t dst, const pointer_t src, size_type n) { std::copy_n(src, n, dst); };
	};

	template<class T>
	struct if_move_traits<true, T>
	{
		using lmove_t = typename std::add_lvalue_reference<T>::type;
		using rmove_t = typename std::add_rvalue_reference<T>::type;
		static void move(lmove_t l, rmove_t r) { l = r; }
	};

	template<class T>
	struct copy_move_traits
	{
		using reference_t = typename std::add_lvalue_reference<T>::type;
		using pointer_t = T*;
		using lmove_t = typename std::add_lvalue_reference<T>::type;
		using rmove_t = typename std::add_rvalue_reference<T>::type;
		static const bool copy_assignable = std::is_copy_constructible<T>::value;
		static const bool move_assignable = std::is_move_assignable<T>::value;
		static void copy(reference_t dst, const reference_t src) { if_copy_traits<copy_assignable, T>::copy(dst, src); };
		static void copy_n(pointer_t dst, const pointer_t src, size_type n) { if_copy_traits<copy_assignable, T>::copy_n(dst, src, n); }
		static void move(lmove_t l, rmove_t r) { if_move_traits<move_assignable, T>::move(l, r); }
	};

	//-------------------------allocator_traits--------------------------------------------------------------------------------

	template<class T, class...Args>
	constexpr void fill_construct_n(T* dst, const size_type size, Args...args)
	{
		for (size_type i = 0; i < size; ++i)
			::new(&dst[i]) T(args...);
	}

	template<class T, class...Args>
	struct func_t;

	template<class T, class...Args>
	struct func_t<T(Args...)>
	{
		using type = T(Args...);
		using ret_t = T;
		using class_t = void;
	};

	template<class T, class U, class...Args>
	struct func_t<T(U::*)(Args...)>
	{
		using type = T(U::*)(Args...);
		using ret_t = T;
		using class_t = U;
	};


	struct has_alloc_t {};
	struct has_realloc_t {};
	struct has_dealloc_t {};
	struct has_construct_t {};
	struct has_constructs_t {};
	struct has_destruct_t {};
	struct has_clear_t {};

	template<class ...Alloc>
	using has_interface_alloc_t = void;

	template<class T, class interface_t, class = void>
	struct has_interface_allocator :std::false_type {};

	template<class T>
	struct has_interface_allocator<T, has_alloc_t, has_interface_alloc_t<decltype(&T::alloc)>> :std::true_type {};
	template<class T>
	struct has_interface_allocator<T, has_dealloc_t, has_interface_alloc_t<decltype(&T::dealloc)>> :std::true_type {};
	template<class T>
	struct has_interface_allocator<T, has_construct_t, has_interface_alloc_t<decltype(&T::construct)>> :std::true_type {};
	template<class T>
	struct has_interface_allocator<T, has_constructs_t, has_interface_alloc_t<decltype(&T::constructs)>> :std::true_type {};
	template<class T>
	struct has_interface_allocator<T, has_destruct_t, has_interface_alloc_t<decltype(&T::destruct)>> :std::true_type {};
	template<class T>
	struct has_interface_allocator<T, has_clear_t, has_interface_alloc_t<decltype(&T::clear)>> :std::true_type {};

	template<class Alloc, class interface_t, bool = has_interface_allocator<Alloc, interface_t>::value>
	struct allocator_func_t
	{
		using pointer = typename func_t<decltype(&Alloc::alloc)>::ret_t;
		using object = typename std::remove_pointer<pointer>::type;

		template<class ...Args>
		constexpr static pointer alloc(Alloc& a, Args... args) { return ::new object(args...); };
		template<class ...Args>
		constexpr static pointer realloc(Alloc& a, Args... args) { return ::new object(args...); };
		template<class T>
		constexpr static void dealloc(Alloc& a, T p) { delete p; };
		template<class T, class...Args>
		constexpr static void construct(Alloc&, T* p, Args...args) { ::new(p) T(args...); };
		template<class T, class...Args>
		constexpr static void constructs(Alloc&, T* p, const size_type size, Args...args) { fill_construct_n(p, size, args...); };
		template<class T>
		constexpr static void destruct(Alloc&, T* p) { p->~T(); };
		constexpr static void clear(Alloc& a) {};
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_alloc_t, true>
	{
		using ret_t = typename func_t<decltype(&Alloc::alloc)>::ret_t;
		template<class ...Args>
		constexpr static ret_t alloc(Alloc& a, Args...args) { return a.alloc(args...); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_realloc_t, true>
	{
		using ret_t = typename func_t<decltype(&Alloc::realloc)>::ret_t;
		template<class ...Args>
		constexpr static ret_t realoc(Alloc& a, Args...args) { return a.realloc(args...); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_dealloc_t, true>
	{
		template<class T>
		constexpr static void dealloc(Alloc& a, T p) { return a.dealloc(p); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_construct_t, true>
	{
		template<class T, class...Args>
		constexpr static void construct(Alloc& a, T* p, Args ...args) { a.construct(p, args...); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_constructs_t, true>
	{
		template<class T, class...Args>
		constexpr static void construct(Alloc& a, T* p, size_type size, Args ...args) { a.construct(p, size, args...); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_destruct_t, true>
	{
		template<class T>
		constexpr static void destruct(Alloc& a, T* p) { a.destruct(p); }
	};

	template<class Alloc>
	struct allocator_func_t<Alloc, has_clear_t, true>
	{
		constexpr static void clear(Alloc& a) { a.clear(); };
	};

	template<class Alloc>
	class IAllocator
	{
	public:
		using func = typename func_t<decltype(&Alloc::alloc)>::type;
		using ret_t = typename func_t<decltype(&Alloc::alloc)>::ret_t;
		using object = typename std::remove_pointer<ret_t>::type;

		using alloc_func_t = allocator_func_t<Alloc, has_alloc_t>;
		using realloc_func_t = allocator_func_t<Alloc, has_realloc_t>;
		using dealloc_func_t = allocator_func_t<Alloc, has_dealloc_t>;
		using construct_func_t = allocator_func_t<Alloc, has_construct_t>;
		using constructs_func_t = allocator_func_t<Alloc, has_constructs_t>;
		using destruct_func_t = allocator_func_t<Alloc, has_destruct_t>;
		using clear_func_t = allocator_func_t<Alloc, has_clear_t>;
	private:

	public:
		template<class ...Args>
		static constexpr ret_t allocate(Alloc& a, Args...args) { return alloc_func_t::alloc(a, args...); };
		template<class ...Args>
		static constexpr ret_t reallocate(Alloc& a, Args... args) { return realloc_func_t::realloc(a, args...); }
		template<class T>
		static constexpr void deallocate(Alloc& a, T n) { dealloc_func_t::dealloc(a, n); };

		template<class T, class ...Args>
		static constexpr void construct(Alloc& a, T* p, Args... arg) { construct_func_t::construct(a, p, arg...); };
		template<class T, class ...Args>
		static constexpr void constructs(Alloc& a, T* p, size_type size, Args... arg) { construct_func_t::constructs(a, p, size, arg...); };
		template<class T>
		static constexpr void destruct(Alloc& a, T* p) { destruct_func_t::destruct(a, p); };
		static constexpr void clear(Alloc& a) { clear_func_t::clear(a); }

		template<class...Args>
		static constexpr Alloc* Get(Args... args) { return ::new Alloc(args...); };
		static constexpr void Release(Alloc* a) { delete a; }
	};

}