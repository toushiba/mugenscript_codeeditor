#pragma once
#include "common/Container.h"
#include "common/String.h"
#include "common/Variant.h"
namespace ResDx
{
	template<class Key, class T>
	using map_t = Container::AsyncHashMap<Key, T>;
	template<class T>
	using vector_t = Container::AsyncVector<T>;
	template<class T>
	using list_t = Container::AsyncList<T>;
	template<class T>
	using array_t = Container::AsyncArray<T>;
	template<class T>
	using queue_t = Container::AsyncQueue<T>;
	template<class T,size_type packet=16>
	using freelist_t = Container::AsyncFreeListEx<T,packet>;
	template<class T>
	using reader_t = typename Container::AsyncFreeListEx<T>::data_t;
	template<class...T>
	using variant_t = Container::Variant<T...>;
	using string_t = Container::String;
}