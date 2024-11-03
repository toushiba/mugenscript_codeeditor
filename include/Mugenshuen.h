#pragma once
#include <unordered_map>
#include <vector>
#include <queue>
#include <stack>
#include "common/FileSystem.h"
#include "common/String.h"
#include "common/Container.h"
#define FREELIST_MAX_NUM_PACKET 64

namespace Mugenshuen
{
	template<class T, size_type packetSize = FREELIST_MAX_NUM_PACKET, template<class>class allocator_t = MemAllocator>
	using freelist_t = Container::AsyncFreeListEx<T, packetSize, allocator_t>;
	template<class Key,class T>
	using map_t = Container::AsyncHashMap<Key,T>;
	template<class T>
	using vector_t = std::vector<T>;
	template<class T>
	using list_t = std::list<T>;
	template<class T>
	using queue_t = std::queue<T>;
	template<class T>
	using array_t = Container::AsyncArray<T>;
	using string_t = Container::String;
	template<class T>
	using stack_t = std::stack<T>;
	template<class T>
	using btree_t = Container::BTree<T, 3>;
	template<class T>
	using delegate_t = Container::Delegate<T>;
	
	template<class T>
	struct Vector2
	{
		T x;
		T y;
	};

	using position_t = Vector2<float>;
	using rotation_t = float;
	using scale_t = Vector2<float>;
	using fontpos_t = Vector2<int>;

	extern const char keyDecide;
	extern const char keyChancel;
	extern const char keyMenu;
	extern const char keyUp;
	extern const char keyDown;
	extern const char keyRight;
	extern const char keyLeft;
}
