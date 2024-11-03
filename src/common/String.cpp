#include "common/String.h"
#include <string.h>
#include <d3d12.h>

const int Container::String::Atoi()
{
	return atoi(str);
}

const double Container::String::Atof()
{
	return atof(str);
}

void Container::String::Itoa(int value)
{
	int digit = 0;
	if (!value)
		digit = 2;
	else if (0 < value)
		digit = log10(value) + 2;
	else
		digit = log10(value * -1) + 3;
	reserve(digit);
	lengh = sprintf_s(str, capacity, "%d", value);
}

Container::String::String(const CHR* s):str(),lengh(),capacity(),allocator()
{
	*this = s;
}

Container::String::String(const String& string):str(),lengh(),capacity(),allocator()
{
	*this = string;
}

Container::String::String(const int value) : str(), lengh(), capacity(0), allocator()
{
	Itoa(value);
}

Container::String::String(String&& string)noexcept
{
	*this = std::move(string);
}

Container::String::String() : str(), lengh(), capacity(), allocator()
{
	*this = "";
}

Container::String::~String()
{
	if (str)
		allocator_traits::deallocate(allocator, str);
	str = nullptr;
}

void Container::String::reserve(size_type capa)
{
	CHR* pStr = this->str;
	this->capacity = capa;
	this->str = allocator_traits::allocate(allocator, this->capacity);

	if (pStr)
		allocator_traits::deallocate(allocator, pStr);
	
}

void Container::String::resize(size_type size)
{
	reserve(size);
	lengh = size;
}

size_type Container::String::Find(char ch)
{
	for(size_type i=0;i<lengh;++i)
		if(str[i]==ch)
			return i;
	return lengh;
}

size_type Container::String::Strlen(const CHR* str)const
{
	return strlen(str) + 1;
}

bool Container::String::operator==(const String& str)const
{
	return strcmp(this->str, str.str) == 0;
}

bool Container::String::operator!=(const String& str)const
{
	return strcmp(this->str, str.str) != 0;
}

Container::String Container::String::operator+(const String& str)const
{
	String rStr;
	size_type len = Strlen(str.str);
	len = lengh + len - 1;
	rStr.reserve(len);
	rStr.lengh = len;
	strcpy_s(rStr.str, len, this->str);
	strcat_s(rStr.str, len, str.str);

	return rStr;
}

void Container::String::operator--()
{
	if (lengh)
	{
		lengh--;
		str[lengh] = '\0';
	}
}

void Container::String::operator+=(const String& str)
{
	size_type len = Strlen(str.str);
	if (capacity < lengh + len)
	{
		CHR* pStr = this->str;
		this->capacity = (lengh + len) - 1;
		this->str = allocator_traits::allocate(allocator, this->capacity);
		strcpy_s(this->str, lengh, pStr);

		if (pStr)
			allocator_traits::deallocate(allocator, pStr);
	}

	strcat_s(this->str, capacity, str.str);
	lengh = lengh + len - 1;
}

void Container::String::operator+=(const int value)
{
	String str(value);
	*this += str;
}

CHR& Container::String::operator[](size_type idx)const
{
	assert(idx<capacity);
	return str[idx];
}

void Container::String::operator=(const CHR* str)
{
	size_type len = Strlen(str);

	if (capacity < len)
		reserve(len);

	strcpy_s(this->str, len, str);
	lengh = len;
}

void Container::String::operator=(const String& string)
{
	*this = string.str;
}

void Container::String::operator=(String&& string)noexcept
{
	str = string.str;
	lengh = string.lengh;
	capacity = string.capacity;
	allocator = std::move(string.allocator);

	string.str = nullptr;
	string.lengh = 0;
	string.capacity = 0;
}

void Container::String::operator=(int value)
{
	Itoa(value);
}

void Container::String::operator=(double value)
{
	reserve(20);
	lengh = sprintf_s(str, capacity, "%lf", value);
}

bool Container::String::operator<(const String& o)const
{
	return strcmp(str, o.str) < 0;
}

bool Container::String::operator>(const String&o)const
{
	return strcmp(str, o.str) > 0;
}
bool Container::String::operator<=(const String& o)const
{
	return strcmp(str, o.str) <= 0;
}

bool Container::String::operator>=(const String& o)const
{
	return strcmp(str, o.str) >= 0;
}
