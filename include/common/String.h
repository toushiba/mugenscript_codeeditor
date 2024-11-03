#pragma once
#include <tchar.h>
#include <string_view>
#include "type.h"
#include "traits.h"
#include "Alloc.h"

typedef char CHR;
namespace Container
{

	class String
	{
	public:
		using allocator_t = DefaultAllocator<CHR>;
		using allocator_traits = Traits::IAllocator<allocator_t>;

		bool operator== (const String& str)const;
		bool operator!=(const String& str)const;
		String operator+(const String& str)const;
		void operator--();
		void operator+=(const String& str);
		void operator+=(const int value);
		CHR& operator[](size_type idx)const;
		void operator=(const CHR* str);
		void operator=(const String&);
		void operator=(String&&)noexcept;
		void operator=(int);
		void operator=(double);
		bool operator<(const String&)const;
		bool operator>(const String&)const;
		bool operator<=(const String&)const;
		bool operator>=(const String&)const;

		size_type Lengh()const { return lengh; };
		size_type Capacity()const{ return capacity; };
		const CHR* C_Str()const { return str; };
		CHR* Data() { return str; }
		const int Atoi();
		const double Atof();
		void Itoa(int);
		void reserve(size_type capacity);
		void resize(size_type size);
		size_type Find(char ch);

		String(const CHR*);
		String(const String&);
		String(const int);
		String(String&&)noexcept;
		String();
		~String();

	private:

		size_type Strlen(const CHR*)const;

		CHR* str = nullptr;
		size_type lengh;
		size_type capacity;
		allocator_t allocator;
	};
}

namespace std
{
	template<>
	struct hash<Container::String>
	{
		using string_t = Container::String;
		size_t operator()(string_t str);

	};
	inline size_t hash<Container::String>::operator()(string_t str)
	{
		std::hash<std::string_view> h;
		return h(str.C_Str());
	}
}