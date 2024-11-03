#pragma once

namespace Container
{

	class none_t
	{
	public:

		template<class T>
		operator T() { return T(); };
		void operator=(const none_t&){}
		none_t(){}
		none_t(const none_t&){}
		~none_t(){}
	};

	template<int n, class T, class...U>
	class get_type_for_param
	{
	public:
		using type = typename get_type_for_param<n - 1, U...>::type;
	};

	template<class T,class...U>
	class get_type_for_param<0, T, U...>
	{
	public:
		using type = T;
	};

	template<class find_type,class T,class...U>
	class find_type_for_param
	{
	public:

		using type = typename find_type_for_param<find_type, U...>::type;
	};

	template<class find_type,class...U>
	class find_type_for_param<find_type,find_type,U...>
	{
	public:

		using type = find_type;
	};

	template<int num,class T,class ...U>
	class count_type_for_param_base
	{
	public:

		static const int value = count_type_for_param_base<num + 1, U...>::value;
	};

	template<int num,class T>
	class count_type_for_param_base<num,T>
	{
	public:
		static const int value = num + 1;
	};

	template<class...T>
	class count_type_for_param
	{
	public:
		static const int value = count_type_for_param_base<0, T...>::value;
	};


	template<class T,class...U>
	class VariantBase
	{
	public:

		T& get() { return value; };
		const T& get()const { return value; }
		VariantBase<U...>& next() { return n; };
		const VariantBase<U...>& next()const { return n; };
		void set(const T& val) { value = val; };
		template<class A>
		void set(const A& val) {};
		template<class O,class...P>
		void operator=(const VariantBase<O, P...>& val) { set(val.get()); next() = val.next(); };
		template<class O>
		void operator=(const VariantBase<O>& val) {};
		void operator=(const VariantBase<T>& val) { value = val.get(); }
		VariantBase() :n(), value() {};
		VariantBase(T val, U...args) :n(args...), value(val) {};

	private:

		VariantBase<U...> n;
		T value;
	};

	template<class T>
	class VariantBase<T>
	{
	public:

		T& get() { return value; };
		const T& get()const { return value; }
		void set(const T& val) { value = val; };
		template<class A>
		void set(const A& val) {};
		size_type size()const { return sizeof(T); };
		template<class...U>
		void operator=(const VariantBase<U...>& val) { set(val.get()); };
		
		VariantBase() : value() {};
		VariantBase(T val) :value(val) {};

	private:
		T value;
	};

	template<int n,class T,class...U>
	class VariantFunction
	{
	public:

		using param_type = typename get_type_for_param<n, T, U...>::type;
		using function_t = VariantFunction<n-1, U...>;

		static param_type& get(VariantBase<T,U...>& base) { return function_t::get(base.next()); };
		static void set(VariantBase<T, U...>& base,const param_type& val) { function_t::set(base.next(),val); };
		static size_type size(VariantBase<T, U...>& base) { return function_t::size(base.next()); };
		static void set_size(size_type size[]) { size[0] = sizeof(T); function_t::set_size(&size[1]); }
	};

	template<class T,class...U>
	class VariantFunction<0, T, U...>
	{
	public:
		using param_type = typename get_type_for_param<0, T, U...>::type;
		
		static param_type& get(VariantBase<T, U...>& base) { return base.get(); };
		static void set(VariantBase<T, U...>& base, const param_type& val) { base.set(val); };
		static size_type size(VariantBase<T, U...>& base) { return base.size(); }
		static void set_size(size_type size[]) { size[0] = sizeof(T); }
	};

	template<class T,class...U>
	class Variant
	{
	public:

		template<int n>
		using param_type = typename get_type_for_param<n, T,U...>::type;
		template<int n>
		using function_t = VariantFunction<n,T, U...>;

		template<int n>
		param_type<n>& get() { return function_t<n>::get(value); };
		template<int n>
		void set(const param_type<n>& val) { function_t<n>::set(value, val); };
		template<class...T>
		void operator=(const Variant<T...>& var) { value = var.value; };
		void set_size() { function_t<count_type_for_param<T, U...>::value - 1>::set_size(typeSize); }

		Variant() :value(),typeSize() { set_size(); };
		Variant(T val, U...args) :value(val, args...),typeSize() { set_size(); }

		VariantBase<T,U...> value;
		size_type typeSize[count_type_for_param<T,U...>::value];
	};
	
	
}

using none_t = Container::none_t;