#pragma once
#include "Alloc.h"
#include "type.h"
#include "traits.h"

#define FREELIST_MAX_NUM_PACKET (1 << 8)

namespace Container
{
	constexpr size_type GetResizeNum(size_type preSize) { return (preSize + nAlign) & (nAlign - 1); };

	template<class T>
	struct destructor
	{
		static void destruct(T& val) { val.~T(); };
	};

	template<>
	struct destructor<int>
	{
		static void destruct(int& val) { val = 0; }
	};

	template<>
	struct destructor<unsigned int>
	{
		static void destruct(unsigned int& val) { val = 0; }
	};

	template<class iterator_t>
	class Iterator;
}

namespace Container
{
	static const size_type hashMapDataArraySize = 5;

	template<class T>
	using hash_t = std::hash<T>;
	template<class T, int n>
	class BTree;

	static const size_type hashMapSizeTable[] =
	{
		31,61,127,241,499,911,1619,2539,4021,6709,9613
	};

	template<class T, class allocator_t = PoolAllocator<T>>
	class FreeList
	{
		using allocator_traits = Traits::IAllocator<allocator_t>;
		allocator_t* allocator;
		size_t count;
	public:
		class elem_t
		{
		public:

			T* operator*();
			T* operator->();
			operator bool();
			T* get();
			void destoroy();
			elem_t(T*, FreeList*);
		private:
			bool valid;
			T* ptr;
			FreeList<T>* list;
		};

		elem_t Get();
		void Release(elem_t);
		void Release(T*);
		size_t size();
		FreeList(size_t blockSize);
		~FreeList() {};
	};

	template<class T, int blockSize>
	class FreeListS
	{
		FreeList<T> list;
		using elem_t = typename FreeList<T>::elem_t;
	public:
		elem_t Get() { return list.Get(); };
		void Release(elem_t e) { list.Release(e); }
		void Release(T* p) { list.Release(p); }
		FreeListS() :list(blockSize) {};
		~FreeListS() {};
	};

	template<class T, class allocator_t = AsyncPoolAllocator<T>>
	class AsyncFreeList
	{
	public:

		using allocator_traits = Traits::IAllocator<allocator_t>;
		using ret_t = typename allocator_traits::ret_t;
		using reader_t = typename allocator_t::Reader;
		using data_t = ret_t;

		template<class...Args>
		ret_t Get(Args...);
		void Release(ret_t);
		size_type Size();

		AsyncFreeList() :size(), allocator(allocator_traits::Get()) {}
		AsyncFreeList(size_type size) :size(), allocator(allocator_traits::Get(size)) {}
		AsyncFreeList(size_type size, size_type align) :size(), allocator(allocator_traits::Get(size, align)) {}
		~AsyncFreeList() { allocator_traits::Release(allocator); }

	private:
		std::atomic<size_type> size;
		allocator_t* allocator;
	};


	template<size_type packetSize, class T, template<class>class allocator_t = MemAllocator>
	class PacketBuffer
	{
	public:

		struct Packet
		{
			bool head;
			T value[packetSize];
			T& operator[](size_type index) { return value[index]; }
			Packet():head(),value(){}
			~Packet(){}
		};

		using PacketListAllocator = allocator_t<Packet*>;
		using PacketAllocator = allocator_t<Packet>;
		using Construct_t = allocator_t<T>;
		using allocator_traits_list = Traits::IAllocator<PacketListAllocator>;
		using allocator_traits_packet = Traits::IAllocator<PacketAllocator>;
		using construct_traits = Traits::IAllocator<Construct_t>;
		template<class U>
		using copy_move_traits = Traits::copy_move_traits<U>;

		template<class...args>
		T& construct(const size_type packetIndex, args...arg);
		template<class...args>
		void constructs(const size_type listIndex, args...arg);
		void destruct(const size_type);
		void reserve(size_type numPacket);
		void clear();
		T* get(size_type index)const;
		size_type index(T* value)const;
		size_type size()const;

		void operator=(const PacketBuffer&);
		void operator=(PacketBuffer&&);
		template<size_type size>
		void operator=(const PacketBuffer<size, T>&);

		T& operator[](const size_type);

		PacketBuffer();
		PacketBuffer(const PacketBuffer&);
		PacketBuffer(PacketBuffer&&);
		~PacketBuffer();

	private:

		void ReservePacketList(size_type size);

		Packet** head;
		size_type sizePacketList;
		size_type capacityPacketList;

		PacketListAllocator packetListAllocator;
		PacketAllocator packetAllocator;
		Construct_t constructor;
	};

	template<class T, size_type packetSize=FREELIST_MAX_NUM_PACKET, template<class>class allocator_t = MemAllocator>
	class AsyncFreeListEx
	{
	public:

		using data_t = T*;
		using index_t = size_type;
		using allocator_traits = Traits::IAllocator<allocator_t<T>>;

		template<class...Args>
		data_t Get(Args...args)
		{
			index_t index = next;
			while (index == -1 || !SetNextIndex(index))
				index = next;

			if (next == -1 && lock.Aquire())
			{
				MemResize();
				lock.Release();
			}

			data_t p = GetValue(index);

			allocator_traits::construct(allocator,p, args...);
			return p;
		}
		void Release(data_t);
		size_type Size();
		void Clear();

		void operator=(const AsyncFreeListEx&);
		void operator=(AsyncFreeListEx&&);

		AsyncFreeListEx();
		AsyncFreeListEx(size_type size);
		AsyncFreeListEx(const AsyncFreeListEx&);
		AsyncFreeListEx(AsyncFreeListEx&&);
		~AsyncFreeListEx() {};

	private:

		bool SetNextIndex(size_type index);
		index_t GetIndex(T* value);
		data_t GetValue(size_type index);
		void MemResize();
		void MemResize(size_type);

		union ListData
		{
			T data;
			index_t next;
			~ListData(){}
		};

		std::atomic<index_t> next;
		PacketBuffer<packetSize,ListData,allocator_t> packetBuffer;
		UniqueLock lock;
		allocator_t<T> allocator;
	};

	template<class T, class allocator_t = DefaultAllocator<T>>
	class RingBuffer
	{
	public:

		using allocator_traits = Traits::IAllocator<allocator_t>;
		using data_t = typename allocator_t::data_t;

		void alloc(size_type);
		void realloc(size_type start, size_type size);
		void dealloc();
		void copy(size_type start, size_type size, const T* src);
		void copy_head(size_type start, size_type size, const T* src);
		size_type get_size();
		T& operator[](size_type index);
		const T& operator[](size_type)const;

		void operator=(const RingBuffer&);
		void operator=(RingBuffer&&);

		RingBuffer();
		RingBuffer(const RingBuffer&);
		RingBuffer(RingBuffer&&);
		~RingBuffer();

	private:

		data_t head;
		size_type size;
		allocator_t allocator;
	};

	template<class T, class allocator_t = DefaultAllocator<T>>
	class AsyncQueue
	{
	public:

		size_type push(const T&);
		void pop();
		bool pop_front(T&);
		T& front();
		T& back();
		bool empty()const;
		size_type size()const;

		void operator=(const AsyncQueue&);
		void operator=(AsyncQueue&&);

		AsyncQueue();
		AsyncQueue(size_type size);
		AsyncQueue(const AsyncQueue&);
		AsyncQueue(AsyncQueue&&);
		~AsyncQueue() {};

	private:


		std::atomic<size_type> start;
		std::atomic<size_type> last;
		std::atomic<size_type> lengh;
		std::atomic<size_type> s;

		ReaderWriterLock lock;
		RingBuffer<T, allocator_t> ringBuffer;
	};

	template<class T, class allocator_t = DefaultAllocator<T>>
	class AsyncVector
	{
	public:

		using allocator_traits = Traits::IAllocator<allocator_t>;

		void push_back(const T& value);
		void push_back(T&& value);
		int remove(const T& value);
		void erase(const T& value);
		void clear();
		void resize(size_type n);
		size_t get_size()const;
		T& back();
		T& operator[](size_type n);
		const T& operator[](size_type n)const;
		void operator=(const AsyncVector&);
		AsyncVector() :allocator(), capacity(), size(), head() { capacity = nAlign; head = allocator_traits::allocate(allocator, capacity); }
		AsyncVector(const AsyncVector&);
		AsyncVector(const AsyncVector&&);
		~AsyncVector();

	private:

		void reserve(size_type capacity);
		void copy(T* src);

		T* head;
		std::atomic<int> size;
		size_type capacity;
		ReaderWriterLock lock;
		SpinLock pushLock;
		allocator_t allocator;
	};

	template<class T, class allocator_t = DefaultAllocator<T>>
	class AsyncArray
	{
	public:

		using allocator_traits = Traits::IAllocator<allocator_t>;
		using copy_move_traits = Traits::copy_move_traits<T>;
		using this_t = AsyncArray<T, allocator_t>;

		void reserve(size_type size);

		size_type size()const;
		void copy(const void* src, size_type size);
		void clear();
		T* data()const;
		T& operator[](size_type idx);
		const T& operator[](size_type)const;
		void operator=(const this_t&);
		void operator=(this_t&&);

		AsyncArray() :head(), capacity(), allocator(), lock() {};
		AsyncArray(size_type size);
		AsyncArray(const this_t&);
		AsyncArray(this_t&&);
		~AsyncArray();

	private:

		T* head;
		size_type capacity;
		allocator_t allocator;
		ReaderWriterLock lock;

	};

	template<int size, class T, class allocator_t = DefaultAllocator<T>>
	class AsyncArray_S
	{
		using base_t = AsyncArray<T, allocator_t>;

		operator base_t& () { return data; };
		operator const base_t& ()const { return data; };

		AsyncArray_S() :data(size) {}

	public:
		base_t data;
	};

	template<class T>
	struct Node
	{
		using this_t = Node<T>;

		T obj;
		std::atomic<Node*> next;
		std::atomic<Node*> prev;

		void set_next(Node*);
		void set_prev(Node*);
		void disconnect();

		Node() :next(), prev() {};
	};

	template<class T, class allocator_t = DefaultAllocator<Node<T>>>
	class AsyncList
	{
	public:

		using allocator_traits = Traits::IAllocator<allocator_t>;
		using data_t = typename allocator_t::data_t;
		using node_t = Node<T>;

		class iterator
		{
		public:

			T& operator*()const { return *node->obj; };
			const T& operator*()const { return *node->obj; };
			iterator& operator++() { if (node->next)node = node->next; return *this; };
			iterator& operator--() { if (node->prev)node = node->prev; return *this; };
			bool operator==(const iterator& it) { return node == it.node; }
			bool operator==(const T& v) { return node->obj == v; }
			operator bool() { return node; }
			iterator(node_t* node) :node(node) {};

		private:
			node_t* node;

		};

		void push_back(const T&);
		void push_front(const T&);
		void pop_back();
		void pop_front();
		void insert(const iterator, const T&);
		void erase(const iterator);
		iterator find(const T&)const;
		iterator begin()const;
		iterator end()const;

		AsyncList() :lock(), allocator(), last(), start(&last) {}

	private:

		SpinLock lock;
		allocator_t allocator;
		node_t last;
		node_t* start;
	};

	template<class First, class Second>
	struct Pair
	{
		First first;
		Second second;
		Pair() :first(), second() {};
		Pair(First first, Second second) :first(first), second(second) {};
		Pair(First first) :first(first), second() {};
		template<class...Args1, class...Args2>
		Pair(Args1...args1, Args2...args2) :first(args1...), second(args2...) {}
	};

	template<class Key, class T>
	struct HashMapNode
	{
		std::atomic<HashMapNode<Key, T>*> next;
		Key key;
		T value;
		HashMapNode() :next(), key(), value() {};
	};

	template<class Key, class T>
	class AsyncMultiMap
	{
	public:

		using node_t = HashMapNode<Key, T>;

		node_t* Push(const Key& key, const T& value);
		void Push(node_t* node);
		bool Erase(const Key& key);
		void Clear();
		node_t* Find(const Key&)const;
		size_type Buket()const;
		node_t* Head()const;
		void operator=(const AsyncMultiMap<Key, T>&);
		AsyncMultiMap() :head(), buket() {};
	private:

		std::atomic<node_t*> head;
		std::atomic<size_type> buket;
	};

	template<class Key, class T, class Hash = hash_t<Key>, class allocator_t = DefaultAllocator<AsyncMultiMap<Key, T>>>
	class AsyncHashMap
	{
	public:

		using this_t = AsyncHashMap<Key, T, Hash, allocator_t>;
		using multimap_t = AsyncMultiMap<Key, T>;
		using node_t = typename multimap_t::node_t;
		using allocator_traits = Traits::IAllocator<allocator_t>;
		using iterator = Iterator<this_t>;

		T& operator[](const Key&);
		const T& get(const Key&)const;
		void erase(const Key&);
		void clear();
		size_type index(const Key&)const;
		node_t* find(const Key&)const;
		node_t* next_node(const node_t*);
		node_t* begin()const;
		void rehash(size_type n);
		void operator=(const AsyncHashMap<Key, T, Hash, allocator_t>&);
		AsyncHashMap(const this_t& map) { *this = map; }
		AsyncHashMap();
		~AsyncHashMap() { clear(); allocator_traits::deallocate(allocator, (multimap_t*)head); }

	private:

		std::atomic<multimap_t*> head;
		node_t endNode;
		size_type size;
		size_type tableIndex;
		size_type buket;
		allocator_t allocator;
		ReaderWriterLock lock;
		Hash hasher;
	};

	template<class T, int N = 2>
	class BTreeNode
	{
	public:
		using this_t = BTreeNode<T, N>;
		static const int n = N * 2 - 1;
		void insert(const T& value);
		bool remove(const T& value);
		void clear();
		void count_equal(const T& min, const T& max,int& v)const;
		void count(const T& min, const T& max, int& v)const;

		int split(int idx);
		void traverse(int& counter, int buffer,T dst[]);
		void traverse(int& counter,const T& min,const T& max, int buffer, T dst[])const;
		void traverse(const T& min, const T& max, void(f)(T&));
		const this_t* find(const T&)const;
		const this_t* find(int&, const T&)const;
		this_t* find(const T&);
		this_t* find(int&, const T&);
		T& operator[](int);
		const T& operator[](int)const;
		void operator=(const this_t&);
		void operator=(this_t&&);
		int index(const T& value);
		T keys[n];
		int numKeys;
		int size;
		bool leaf;
		BTreeNode* child[n + 1];
		BTreeNode() :keys(), child(),numKeys(),size(), leaf() {}
	private:
		void InsertNode(const T& value);
		void fill(int idx);
		void BorrowFromPrev(int idx);
		void BorrowFromNext(int idx);
		void RemoveFromLeaf(int idx);
		void RemoveFromNonLeaf(int idx);
		T& GetPred(int idx);
		T& GetSucc(int idx);
		void marge(int idx);
	
	};

	template<class T,int n=2>
	class BTree
	{
	public:
		using node_t = BTreeNode<T, n>;
		const node_t* find(const T&)const;
		const node_t* find(int&,const T&)const;
		node_t* find(const T&);
		node_t* find(int&, const T&);
		void insert(const T&);
		void remove(const T&);
		void traverse(int buffer,T dst[]);
		void traverse(const T& min,const T& max,int buffer, T dst[])const;
		void traverse(const T& min, const T& max, void(f)(T&));
		int count_equal(const T& min, const T& max)const;
		int count(const T& min, const T& max)const;
		int size()const;
		void clear();
		T& operator[](int);
		const T& operator[](int)const;
		void operator=(const BTree& o);
		void operator=(BTree&& o);
		//void erase(const T&);
		BTree();
		BTree(const BTree&);
		BTree(BTree&&);
		~BTree();
	private:
		node_t* root;
	};

	template<class ret, class...arg>
	class DelegateBase;

	template<class ret, class...arg>
	class DelegateBase<ret(arg...)>
	{
	public:
		virtual ret operator()(arg...) = 0;
	};

	template<class ret, class...arg>
	class Delegate;

	template<class ret, class obj, class...arg>
	class Delegate<ret(obj::*)(arg...)> :public DelegateBase<ret(arg...)>
	{
	public:

		using func_t = ret(obj::*)(arg...);
		using this_t = Delegate<ret, obj, arg...>;
		ret operator()(arg...a)override { return (o->*f)(a...); };
		void operator=(const this_t& other) { o = other.o; f = other.f; }
		void set(obj* o, func_t f) { this->o = o; this->f = f; }
		Delegate() :o(), f() {};

	private:
		obj* o;
		func_t f;
	};


	template<class T, class allocator_t>
	inline T* FreeList<T, allocator_t>::elem_t::operator*()
	{
		if (valid)
			return ptr;
		return nullptr;
	}

	template<class T, class allocator_t>
	inline T* FreeList<T, allocator_t>::elem_t::operator->()
	{
		if (valid)
			return ptr;
		return nullptr;
	}

	template<class T, class allocator_t>
	inline FreeList<T, allocator_t>::elem_t::operator bool()
	{
		return valid;
	}

	template<class T, class allocator_t>
	inline T* FreeList<T, allocator_t>::elem_t::get()
	{
		return ptr;
	}

	template<class T, class allocator_t>
	inline void FreeList<T, allocator_t>::elem_t::destoroy()
	{
		valid = false;
		list->Release(this);
	}

	template<class T, class allocator_t>
	inline FreeList<T, allocator_t>::elem_t::elem_t(T* ptr, FreeList* list) :ptr(ptr), list(list), valid(true)
	{
	}

	template<class T, class allocator_t>
	inline typename FreeList<T, allocator_t>::elem_t FreeList<T, allocator_t>::Get()
	{
		return elem_t(allocator_traits::allocate(*allocator), this);
	}

	template<class T, class allocator_t>
	inline void FreeList<T, allocator_t>::Release(elem_t p)
	{
		allocator_traits::deallocate(*allocator, *p);
	}

	template<class T, class allocator_t>
	inline void FreeList<T, allocator_t>::Release(T* p)
	{
		allocator_traits::deallocate(*allocator, p);
	}

	template<class T, class allocator_t>
	inline size_t FreeList<T, allocator_t>::size()
	{
		return allocator->count() - count;
	}

	template<class T, class allocator_t>
	inline FreeList<T, allocator_t>::FreeList(size_t blockSize) : allocator(allocator_traits::Get(blockSize)), count(blockSize)
	{
	}



	template<class T, class allocator_t>
	template<class ...Args>
	inline typename AsyncFreeList<T, allocator_t>::ret_t AsyncFreeList<T, allocator_t>::Get(Args...args)
	{
		++size;
		auto data = allocator_traits::allocate(*allocator);
		allocator->construct(data, args...);
		return data;
	}

	template<class T, class allocator_t>
	inline void AsyncFreeList<T, allocator_t>::Release(ret_t a)
	{
		--size;
		allocator_traits::deallocate(*allocator, a);
	}

	template<class T, class allocator_t>
	inline size_type AsyncFreeList<T, allocator_t>::Size()
	{
		return size;
	}


	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::alloc(size_type size)
	{
		head = allocator_traits::allocate(allocator, size);
		this->size = size;
	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::realloc(size_type start, size_type size)
	{
		auto newPtr = allocator_traits::allocate(allocator, size);
		auto prePtr = head;
		auto preSize = this->size;
		head = newPtr;
		this->size = size;
		copy_head(start, preSize, prePtr);
		delete[] prePtr;
	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::dealloc()
	{
		allocator_traits::deallocate(allocator, head);
		size = 0;
		head = nullptr;
	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::copy(size_type start, size_type size, const T* src)
	{
		int len = (int)(start + size - this->size);

		if (len > 0)
		{
			memcpy(&head[start], &src[start], (size - start) * sizeof(T));
			memcpy(&head[size], &src[0], (this->size - size) * sizeof(T));
			memcpy(&head[0], &src[this->size - size], len * sizeof(T));
		}
		else
		{
			len = start;
			memcpy(&head[size], &src[0], start * sizeof(T));
			memcpy(&head[start], &src[start], (size - len) * sizeof(T));
		}

	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::copy_head(size_type start, size_type size, const T* src)
	{
		assert(this->size >= size);
		memcpy(&head[0], &src[start], (size - start) * sizeof(T));
		memcpy(&head[size - start], &src[0], (start) * sizeof(T));
	}

	template<class T, class allocator_t>
	inline size_type RingBuffer<T, allocator_t>::get_size()
	{
		return size;
	}

	template<class T, class allocator_t>
	inline T& RingBuffer<T, allocator_t>::operator[](size_type index)
	{
		return head[index % size];
	}

	template<class T, class allocator_t>
	inline const T& RingBuffer<T, allocator_t>::operator[](size_type index) const
	{
		return head[index];
	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::operator=(const RingBuffer& o)
	{
		alloc(o.size);
		memcpy_s(head, o.size + 1, o.head, o.size);
		size = o.size;
	}

	template<class T, class allocator_t>
	inline void RingBuffer<T, allocator_t>::operator=(RingBuffer&& o)
	{
		head = std::move(o.head);
		size = std::move(o.size);
		o.head = nullptr;
		o.size = 0;
	}

	template<class T, class allocator_t>
	inline RingBuffer<T, allocator_t>::RingBuffer():head(),size()
	{
	}

	template<class T, class allocator_t>
	inline RingBuffer<T, allocator_t>::RingBuffer(const RingBuffer& o)
	{
		*this = o;
	}

	template<class T, class allocator_t>
	inline RingBuffer<T, allocator_t>::RingBuffer(RingBuffer&& o)
	{
		*this = std::move(o);
	}

	template<class T, class allocator_t>
	inline RingBuffer<T, allocator_t>::~RingBuffer()
	{
		if (head)
			dealloc();
	}

	template<class T, class allocator_t>
	inline size_type AsyncQueue<T, allocator_t>::push(const T& a)
	{
		size_type lst = 0;
		{
			ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
			lst = last.load();
			while (lengh >= s - 1 && start == last || !last.compare_exchange_weak(lst, (lst + 1) % s))
			{
				std::this_thread::yield();
				lst = last.load();
			}

			ringBuffer[lst] = a;

			auto preLengh = lengh.load();
			while (!lengh.compare_exchange_weak(preLengh, preLengh + 1))
			{
				std::this_thread::yield();
				preLengh = lengh.load();
			}
		}

		if (lengh >= s - 1)
		{
			ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
			auto preLengh = lengh.load();
			if (lengh >= s - 1 && lengh.compare_exchange_weak(preLengh, 0))
			{
				size_type newSize = (s + nAlign) & ~(nAlign - 1);
				ringBuffer.realloc(start, newSize);
				s = newSize;
				last = (preLengh) % s;
				start = 0;
				lengh += preLengh;
			}
		}
		return lst + 1;
	}

	template<class T, class allocator_t>
	inline void AsyncQueue<T, allocator_t>::pop()
	{
		auto str = start.load();
		auto len = lengh.load();

		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		if (lengh > 0 && lengh.compare_exchange_weak(len, len - 1))
		{
			while (!start.compare_exchange_weak(str, (str + 1) % s))
			{
				std::this_thread::yield();
				str = start.load();
			}
		}
	}

	template<class T, class allocator_t>
	inline bool AsyncQueue<T, allocator_t>::pop_front(T& a)
	{
		auto str = start.load();
		auto len = lengh.load();

		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		if (lengh > 0 && lengh.compare_exchange_weak(len, len - 1))
		{
			while (!start.compare_exchange_weak(str, (str + 1) % s))
			{
				std::this_thread::yield();
				str = start.load();
			}

			a = ringBuffer[str];

			return true;
		}

		return false;
	}

	template<class T, class allocator_t>
	inline T& AsyncQueue<T, allocator_t>::front()
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return ringBuffer[start];
	}

	template<class T, class allocator_t>
	inline T& AsyncQueue<T, allocator_t>::back()
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return ringBuffer[last - 1];
	}

	template<class T, class allocator_t>
	inline bool AsyncQueue<T, allocator_t>::empty()const
	{
		return lengh == 0;
	}

	template<class T, class allocator_t>
	inline size_type AsyncQueue<T, allocator_t>::size()const
	{
		return s;
	}

	template<class T, class allocator_t>
	inline void AsyncQueue<T, allocator_t>::operator=(const AsyncQueue& o)
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		start = o.start.load();
		last = o.last.load();
		lengh = o.lengh.load();
		s = o.s.load();
		ringBuffer = o.ringBuffer;
	}

	template<class T, class allocator_t>
	inline void AsyncQueue<T, allocator_t>::operator=(AsyncQueue&& o)
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		ScopeLock<ReaderWriterLock::Writer> l2(o.lock.writer);

		start = std::move(o.start.load());
		last = std::move(o.last.load());
		lengh = std::move(o.lengh.load());
		s = std::move(o.s.load());
		ringBuffer = std::move(o.ringBuffer);
		o.start = 0;
		o.last = 0;
		o.lengh = 0;
		o.s = 0;
	}

	template<class T, class allocator_t>
	inline AsyncQueue<T, allocator_t>::AsyncQueue() :start(), last(), lengh(), s(nAlign), lock(), ringBuffer()
	{
		ringBuffer.alloc(nAlign);
	}

	template<class T, class allocator_t>
	inline AsyncQueue<T, allocator_t>::AsyncQueue(size_type size) :start(), last(), lengh(), size(size), lock(), ringBuffer()
	{
		ringBuffer.alloc(size);
	}

	template<class T, class allocator_t>
	inline AsyncQueue<T, allocator_t>::AsyncQueue(const AsyncQueue& o)
	{
		*this = o;
	}

	template<class T, class allocator_t>
	inline AsyncQueue<T, allocator_t>::AsyncQueue(AsyncQueue&& o)
	{
		*this = std::move(o);
	}

	template<class T>
	inline void Node<T>::set_next(Node* node)
	{
		next = node;
		node->prev = this;
	}

	template<class T>
	inline void Node<T>::set_prev(Node* node)
	{
		prev = node;
		node->next = this;
	}

	template<class T>
	inline void Node<T>::disconnect()
	{
		if (prev)
			prev->next = next;
		if (next)
			next->prev = prev;

		next = nullptr;
		prev = nullptr;
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::push_back(const T& v)
	{
		node_t* data = allocator_traits::allocate(allocator, 1);
		data->obj = v;

		ScopeLock<SpinLock> l(lock);
		data->set_next(last);

		if (start == &last)
			start = data;
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::push_front(const T& v)
	{
		node_t* data = allocator_traits::allocate(allocator, 1);
		data->obj = v;
		data->prev = nullptr;

		ScopeLock<SpinLock> l(lock);
		data->set_next(start);
		start = data;
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::pop_back()
	{
		node_t* node = nullptr;
		lock.Aquire();

		if (start != last)
		{
			node = last.prev;
			node->disconnect();
		}

		if (node == start)
			start = last;

		lock.Release();
		if (node)
			allocator_traits::deallocate(allocator, node);
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::pop_front()
	{
		node_t* node = nullptr;
		lock.Aquire();

		if (start != last)
		{
			node = start->next;
			start = start->next;
		}

		lock.Release();
		if (node)
			allocator_traits::deallocate(allocator, node);
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::insert(const iterator it, const T& v)
	{
		node_t* next = &*it;
		node_t* newNode = allocator_traits::allocate(allocator, 1);
		newNode->obj = v;
		lock.Aquire();

		newNode->set_next(next);
		if (next->prev)
			newNode->set_prev(next->prev);

		if (start == next)
			start = newNode;

		lock.Release();
	}

	template<class T, class allocator_t>
	inline void AsyncList<T, allocator_t>::erase(const iterator it)
	{
		node_t* node = &*it;

		lock.Aquire();
		if (node == start)
			start = node->next;
		else if (node == last)
			last = node->prev;
		node->disconnect();
		lock.Release();
	}

	template<class T, class allocator_t>
	inline typename AsyncList<T, allocator_t>::iterator AsyncList<T, allocator_t>::find(const T& v) const
	{
		iterator i = begin();

		while (i)
		{
			if (i == v)
				break;
			i++;
		}
		return i;
	}

	template<class T, class allocator_t>
	inline typename AsyncList<T, allocator_t>::iterator AsyncList<T, allocator_t>::begin() const
	{
		return start;
	}

	template<class T, class allocator_t>
	inline typename AsyncList<T, allocator_t>::iterator AsyncList<T, allocator_t>::end() const
	{
		return last;
	}

	template<class Key, class T>
	inline typename AsyncMultiMap<Key, T>::node_t* AsyncMultiMap<Key, T>::Push(const Key& key, const T& value)
	{
		node_t* newNode = new node_t;
		node_t* h = head;

		newNode->key = key;
		newNode->value = value;
		newNode->next = head.load();

		while (!head.compare_exchange_weak(h, newNode))
		{
			std::this_thread::yield();
			h = head;
			newNode->next = h;
		}
		buket++;

		return newNode;
	}

	template<class Key, class T>
	inline void AsyncMultiMap<Key, T>::Push(node_t* node)
	{
		node_t* h = head;
		node->next = h;

		while (!head.compare_exchange_weak(h, node))
		{
			std::this_thread::yield();
			h = head;
			node->next = h;
		}
	}

	template<class Key, class T>
	inline bool AsyncMultiMap<Key, T>::Erase(const Key& key)
	{
		node_t* node = head;
		node_t* nextNode = nullptr;

		if (!node)
			return false;

		if (node->key == key)
		{
			if (head.compare_exchange_weak(node, node->next))
			{
				delete node;
				buket--;
				return true;
			}

			node = head;
		}

		while (node)
		{
			nextNode = node->next;
			if (nextNode && nextNode->key == key)
			{
				while (!node->next.compare_exchange_weak(nextNode, nextNode->next))
				{
					std::this_thread::yield();
					nextNode = node->next;
				}

				delete nextNode;
				buket--;
				return true;
			}
			node = nextNode;
		}

		return false;
	}

	template<class Key, class T>
	inline void AsyncMultiMap<Key, T>::Clear()
	{
		node_t* node = head;
		node_t* next = nullptr;

		while (!head.compare_exchange_weak(node, nullptr))
		{
			std::this_thread::yield();
			node = head;
		}

		if (!node)
			return;

		next = node->next;

		while (next)
		{
			delete node;
			node = next;
			next = node->next;
		}

		delete node;
		head = nullptr;
		buket = 0;
	}

	template<class Key, class T>
	inline typename AsyncMultiMap<Key, T>::node_t* AsyncMultiMap<Key, T>::Find(const Key& key)const
	{
		node_t* node = head;

		while (node)
		{
			if (node->key == key)
				return node;
			node = node->next;
		}

		return nullptr;
	}

	template<class Key, class T>
	inline size_type AsyncMultiMap<Key, T>::Buket()const
	{
		return buket;
	}

	template<class Key, class T>
	inline typename AsyncMultiMap<Key, T>::node_t* AsyncMultiMap<Key, T>::Head() const
	{
		return head;
	}

	template<class Key, class T>
	inline void AsyncMultiMap<Key, T>::operator=(const AsyncMultiMap<Key, T>& map)
	{
		Clear();

		node_t* node = map.head;
		node_t* nextNode = nullptr;
		while (node)
		{
			node_t* newNode = new node_t;
			newNode->key = node->key;
			newNode->value = node->value;
			if (!head)
			{
				head = newNode;
				nextNode = head;
			}
			else
			{
				nextNode->next = newNode;
			}
			node = node->next;

		}
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline T& AsyncHashMap<Key, T, Hash, allocator_t>::operator[](const Key& key)
	{
		size_type idx = index(key);
		node_t* node = find(key);

		lock.reader.Aquire();
		if (!node)
		{
			node = head[index(key)].Push(key, T());
			if (buket < head[index(key)].Buket())
				buket = head[index(key)].Buket();
		}
		lock.reader.Release();

		if (head[index(key)].Buket() > 5)
		{
			if (tableIndex < _countof(hashMapSizeTable) - 1)
			{
				tableIndex++;
			}
			else
			{
				assert(false);
			}

			rehash(hashMapSizeTable[tableIndex]);
		}

		return node->value;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline const T& AsyncHashMap<Key, T, Hash, allocator_t>::get(const Key& key) const
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		node_t* node = head[index(key)].Find(key);
		assert(node);
		return node->value;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline void AsyncHashMap<Key, T, Hash, allocator_t>::erase(const Key& key)
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		head[index(key)].Erase(key);
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline void AsyncHashMap<Key, T, Hash, allocator_t>::clear()
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		for (int i = 0; i < size; ++i)
		{
			head[i].Clear();
		}
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline size_type AsyncHashMap<Key, T, Hash, allocator_t>::index(const Key& key)const
	{
		Hash h = hasher;
		return h(key) % size;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline typename AsyncHashMap<Key, T, Hash, allocator_t>::node_t* AsyncHashMap<Key, T, Hash, allocator_t>::find(const Key& key)const
	{
		node_t* node;

		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		node = head[index(key)].Find(key);
		return node;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline typename AsyncHashMap<Key, T, Hash, allocator_t>::node_t* AsyncHashMap<Key, T, Hash, allocator_t>::next_node(const node_t* node)
	{
		if (!node)
			return nullptr;

		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		if (node->next)
			return node->next;

		for (size_type i = index(node->key) + 1; i < size; ++i)
			if (head[i].Buket())
				return head[i].Head();

		return nullptr;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline typename AsyncHashMap<Key, T, Hash, allocator_t>::node_t* AsyncHashMap<Key, T, Hash, allocator_t>::begin()const
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		for (size_type i = 0; i < size; ++i)
			if (head[i].Buket())
				return head[i].Head();
		return nullptr;
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline void AsyncHashMap<Key, T, Hash, allocator_t>::rehash(size_type n)
	{
		multimap_t* newHead = allocator_traits::allocate(allocator, n);
		multimap_t* currentMap = nullptr;
		node_t* node = nullptr;
		node_t* next = nullptr;
		size_type index = 0;

		lock.writer.Aquire();

		buket = 0;
		for (size_type i = 0; i < size; ++i)
		{
			currentMap = &head[i];
			node = currentMap->Head();

			while (node)
			{
				size_type newBuket = 0;

				next = node->next;
				index = hasher(node->key) % n;
				newHead[index].Push(node);
				newBuket = newHead[index].Buket();

				buket = buket < newBuket ? newBuket : buket;

				node = next;
			}

		}

		currentMap = head;
		head = newHead;
		size = n;
		lock.writer.Release();

		allocator_traits::deallocate(allocator, currentMap);

	}

	template<class Key, class T, class Hash, class allocator_t>
	inline void AsyncHashMap<Key, T, Hash, allocator_t>::operator=(const AsyncHashMap<Key, T, Hash, allocator_t>& map)
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);

		size = map.size;
		tableIndex = map.tableIndex;
		buket = map.buket;
		head = allocator_traits::allocate(allocator, map.size);
		for (size_type i = 0; i < size; ++i)
		{
			head[i] = map.head[i];
		}
	}

	template<class Key, class T, class Hash, class allocator_t>
	inline AsyncHashMap<Key, T, Hash, allocator_t>::AsyncHashMap() :lock(), tableIndex(), size(), allocator(), head(), hasher()
	{
		size = hashMapSizeTable[tableIndex];
		head = allocator_traits::allocate(allocator, size);
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::push_back(const T& value)
	{
		int index = size;
		pushLock.Aquire();
		while (index >= capacity || !size.compare_exchange_weak(index, index + 1))
		{
			//std::this_thread::yield();
			index = size;
		}
		pushLock.Release();

		int preSize = size;

		if (capacity <= size && size.compare_exchange_weak(preSize, preSize + 1))
		{
			size--;
			reserve((capacity + nAlign) & ~(nAlign - 1));
		}
		allocator_traits::construct(allocator, &head[index]);
		head[index] = value;
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::push_back(T&& value)
	{
		int index = size;
		pushLock.Aquire();
		while (index >= capacity || !size.compare_exchange_weak(index, index + 1))
		{
			//std::this_thread::yield();
			index = size;
		}
		pushLock.Release();

		int preSize = size;

		if (capacity <= size && size.compare_exchange_weak(preSize, preSize + 1))
		{
			size--;
			reserve((capacity + nAlign) & ~(nAlign - 1));
		}
		allocator_traits::construct(allocator, &head[index]);
		head[index] = std::move(value);
	}

	template<class T, class allocator_t>
	inline int AsyncVector<T, allocator_t>::remove(const T& value)
	{
		int preSize = size;
		int count = 0;

		for (int i = 0; i < preSize - 1; ++i)
		{
			if (head[i] == value)
			{
				ScopeLock<SpinLock> l(pushLock);

				lock.writer.Aquire();
				destructor<T>::destruct(head[i]);
				memmove_s(
					(void*)&head[i],
					(size - i) * sizeof(T),
					(void*)&head[i + 1],
					(size - i - 1) * sizeof(T)
				);
				size--;
				lock.writer.Release();

				count++;
				i--;
			}
		}

		if (head[preSize - 1] == value)
		{
			ScopeLock<SpinLock> l(pushLock);
			lock.writer.Aquire();
			size--;
			destructor<T>::destruct(head[preSize - 1]);
			lock.writer.Release();
		}

		return count;
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::erase(const T& value)
	{
		int count = remove(value);
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::clear()
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		capacity = 0;
		size = 0;
		allocator_traits::deallocate(allocator, head);
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::resize(size_type n)
	{
		if (n < capacity)
		{
			size = n;
		}
		else
		{
			reserve((n + nAlign) & ~(nAlign - 1));
		}
	}

	template<class T, class allocator_t>
	inline size_t AsyncVector<T, allocator_t>::get_size()const
	{
		return size;
	}

	template<class T, class allocator_t>
	inline T& AsyncVector<T, allocator_t>::back()
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return head[size - 1];
	}

	template<class T, class allocator_t>
	inline T& AsyncVector<T, allocator_t>::operator[](size_type n)
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return head[n];
	}

	template<class T, class allocator_t>
	inline const T& AsyncVector<T, allocator_t>::operator[](size_type n) const
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return head[n];
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::operator=(const AsyncVector& vector)
	{
		reserve(vector.capacity);
		memcpy_s(head, capacity * sizeof(T) + 1, vector.head, capacity * sizeof(T));
		size = vector.size.load();
	}

	template<class T, class allocator_t>
	inline AsyncVector<T, allocator_t>::AsyncVector(const AsyncVector& vector) :head(), size(), capacity(), allocator()
	{
		*this = vector;
	}

	template<class T, class allocator_t>
	inline AsyncVector<T, allocator_t>::AsyncVector(const AsyncVector&& vector) :head(), size(), capacity(), allocator()
	{
		ScopeLock<ReaderWriterLock::Writer> l(vector.lock.writer);
		head = vector.head;
		size = vector.size;
		capacity = vector.capacity;

		vector.head = nullptr;
		vector.size = 0;
		vector.capacity = 0;
	}

	template<class T, class allocator_t>
	inline AsyncVector<T, allocator_t>::~AsyncVector()
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		if (head)
			allocator_traits::deallocate(allocator, head);
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::reserve(size_type capacity)
	{
		if (this->capacity > capacity)
			return;

		T* newHead = allocator_traits::allocate(allocator, capacity);
		allocator_traits::constructs(allocator, newHead,capacity);
		lock.writer.Aquire();

		T* preHead = head;
		head = newHead;
		copy(preHead);
		if (preHead)
			allocator_traits::deallocate(allocator, preHead);
		this->capacity = capacity;
		lock.writer.Release();
	}

	template<class T, class allocator_t>
	inline void AsyncVector<T, allocator_t>::copy(T* src)
	{
		if (!src)
			return;

		for (size_type i = 0; i < size; ++i)
			head[i] = src[i];
	}

	template<class T, class allocator_t>
	inline void AsyncArray<T, allocator_t>::reserve(size_type size)
	{
		T* preHead = head;
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		head = allocator_traits::allocate(allocator, size);
		capacity = size;
		if (preHead)
		{
			allocator_traits::deallocate(allocator, preHead);
		}
	}

	template<class T, class allocator_t>
	inline size_type AsyncArray<T, allocator_t>::size()const
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		return capacity;
	}


	template<class T, class allocator_t>
	inline void AsyncArray<T, allocator_t>::copy(const void* src, size_type size)
	{
		assert(size <= capacity);
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		memcpy_s(head, capacity * sizeof(T) + 1, src, size * sizeof(T));
	}

	template<class T, class allocator_t>
	inline void AsyncArray<T, allocator_t>::clear()
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		allocator_traits::deallocate(allocator, head);
		head = nullptr;
		capacity = 0;
	}

	template<class T, class allocator_t>
	inline T* AsyncArray<T, allocator_t>::data() const
	{
		return head;
	}

	template<class T, class allocator_t>
	inline T& AsyncArray<T, allocator_t>::operator[](size_type idx)
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		assert(idx < capacity);
		return head[idx];
	}

	template<class T, class allocator_t>
	inline const T& AsyncArray<T, allocator_t>::operator[](size_type idx) const
	{
		ScopeLock<ReaderWriterLock::Reader> l(lock.reader);
		assert(idx < capacity);
		return head[idx];
	}

	template<class T, class allocator_t>
	inline void AsyncArray<T, allocator_t>::operator=(const this_t& src)
	{
		ScopeLock<ReaderWriterLock::Writer> l(lock.writer);
		size_type size = src.size();
		head = allocator_traits::allocate(allocator, src.capacity);
		copy_move_traits::copy_n(head, src.head, size);
		capacity = src.capacity;
	}

	template<class T, class allocator_t>
	inline void AsyncArray<T, allocator_t>::operator=(this_t&& src)
	{
		ScopeLock<ReaderWriterLock::Writer> l(src.lock.writer);
		ScopeLock<ReaderWriterLock::Writer> l2(lock.writer);
		head = src.head;
		capacity = src.capacity;
		src.head = nullptr;
		src.capacity = 0;
	}

	template<class T, class allocator_t>
	inline AsyncArray<T, allocator_t>::AsyncArray(size_type size) :head(), capacity(), allocator(), lock()
	{
		reserve(size);
	}

	template<class T, class allocator_t>
	inline AsyncArray<T, allocator_t>::AsyncArray(const this_t& src)
	{
		*this = src;
	}

	template<class T, class allocator_t>
	inline AsyncArray<T, allocator_t>::AsyncArray(this_t&& src)
	{
		*this = std::move(src);
	}

	template<class T, class allocator_t>
	inline AsyncArray<T, allocator_t>::~AsyncArray()
	{
		allocator_traits::deallocate(allocator, head);
	}

	//template<class T,size_type packetSize,template<class>class allocator_t>
	//template<class ...Args>
	//inline typename AsyncFreeListEx<T,packetSize, allocator_t>::data_t AsyncFreeListEx<T,packetSize, allocator_t>::Get(Args ...args)
	

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::Release(data_t value)
	{
		allocator_traits::destruct(allocator, value);
		size_type index = GetIndex(value);
		ListData* data = (ListData*)GetValue(index);
		index_t nextIndex = next.load();
		data->next = nextIndex;

		while (!next.compare_exchange_weak(nextIndex, index))
		{
			nextIndex = next.load();
			data->next = nextIndex;
		}
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline size_type AsyncFreeListEx<T,packetSize, allocator_t>::Size()
	{
		return packetBuffer.size();
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::Clear()
	{
		while (!lock.Aquire())
			std::this_thread::yield();
		packetBuffer.clear();
		next = 0;
		lock.Release();
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::operator=(const AsyncFreeListEx& o)
	{
		while (!lock.Aquire())
			std::this_thread::yield();

		next = o.next;
		packetBuffer = o.packetBuffer;

		lock.Release();
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::operator=(AsyncFreeListEx&& o)
	{
		while (!lock.Aquier())
			std::this_thread::yield();
		while (!o.lock.Aquier())
			std::this_thread::yield();

		next = std::move(o.next);
		packetBuffer = std::move(o.packetBuffer);
		o.next = -1;

		lock.Release();
		o.lock.Release();
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline AsyncFreeListEx<T,packetSize, allocator_t>::AsyncFreeListEx():next(),packetBuffer(),lock()
	{
		MemResize();
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline AsyncFreeListEx<T,packetSize, allocator_t>::AsyncFreeListEx(size_type size) :next(), packetBuffer(), lock()
	{
		MemResize(size);
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline AsyncFreeListEx<T,packetSize, allocator_t>::AsyncFreeListEx(const AsyncFreeListEx& o)
	{
		*this = o;
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline AsyncFreeListEx<T,packetSize, allocator_t>::AsyncFreeListEx(AsyncFreeListEx&& o)
	{
		*this = std::move(o);
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline bool AsyncFreeListEx<T,packetSize, allocator_t>::SetNextIndex(size_type index)
	{
		return next.compare_exchange_weak(index,packetBuffer[index].next);
	}

	template<class T, size_type packetSize, template<class> class allocator_t>
	inline typename AsyncFreeListEx<T,packetSize, allocator_t>::index_t AsyncFreeListEx<T,packetSize, allocator_t>::GetIndex(T* value)
	{
		return packetBuffer.index((ListData*)value);
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline typename AsyncFreeListEx<T,packetSize, allocator_t>::data_t AsyncFreeListEx<T,packetSize, allocator_t>::GetValue(size_type index)
	{
		return &packetBuffer[index].data;
	}

	template<class T, size_type packetSize, template<class> class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::MemResize()
	{
		MemResize(1);
	}

	template<class T, size_type packetSize, template<class>class allocator_t>
	inline void AsyncFreeListEx<T,packetSize, allocator_t>::MemResize(size_type numPacket)
	{
		size_type preSize = packetBuffer.size();
		packetBuffer.reserve(numPacket);
		size_type curSize = packetBuffer.size();

		for (size_type i = preSize; i < curSize; ++i)
		{
			packetBuffer[i].next = i + 1;
		}
		packetBuffer[curSize - 1].next = -1;
		next = preSize;
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline PacketBuffer<packetSize, T, allocator_t>::PacketBuffer() :head(),capacityPacketList(), sizePacketList(), packetAllocator()
	{
	}
	template<size_type packetSize, class T, template<class> class allocator_t>
	template<class ...args>
	inline T& PacketBuffer<packetSize, T, allocator_t>::construct(const size_type idx, args ...arg)
	{
		construct_traits::construct(constructor, get(idx), arg);
	}
	template<size_type packetSize, class T, template<class>class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::destruct(const size_type idx)
	{
		allocator_traits_packet::destruct(get(idx));
	}
	template<size_type packetSize, class T, template<class>class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::reserve(size_type numPacket)
	{
		if (capacityPacketList < sizePacketList + numPacket)
			ReservePacketList(sizePacketList + numPacket);
		Packet* ptr = allocator_traits_packet::allocate(packetAllocator, numPacket);
		for (size_type i = 0; i < numPacket; ++i)
		{
			ptr[i].head = false;
			head[sizePacketList++] = &ptr[i];
		}
		ptr[0].head = true;
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::clear()
	{
		if (!head)
			return;
		for (size_type i = 0; i < sizePacketList; ++i)
			if (head[i]->head)
				allocator_traits_packet::deallocate(packetAllocator, head[i]);
		sizePacketList = 0;
	}


	template<size_type packetSize, class T, template<class> class allocator_t>
	inline T* PacketBuffer<packetSize, T, allocator_t>::get(size_type index)const
	{
		size_type list = index / packetSize;
		size_type packet = index % packetSize;
		assert(head[list]);
		return &(*head[list])[packet];
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline size_type PacketBuffer<packetSize, T, allocator_t>::index(T* value)const
	{
		for (size_type i = 0; i < sizePacketList; ++i)
		{
			T* first = &(*head[i])[0];
			T* last = &first[packetSize - 1];
			if (first <= value && value <= last)
				return (value - first) + packetSize * i;

		}
		assert(false);
		return -1;
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::operator=(const PacketBuffer& o)
	{
		assert(copy_move_traits::copy_assignable);
		size_type size = o.sizePacketList;
		ReservePacketList(size);
		capacityPacketList = size;
		reserve(size);
		for (size_type i = 0; i < size; ++i)
			copy_move_traits<Packet>::copy(*head[i], *o.head[i]);
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	template<size_type otherPacketSize>
	inline void PacketBuffer<packetSize, T, allocator_t>::operator=(const PacketBuffer<otherPacketSize, T>& o)
	{
		assert(copy_move_traits<T>::copy_assignable);
		size_type size = o.size() / packetSize + 1;
		size_type capacity = o.size();
		ReservePacketList(size);
		capacityPacketList = size;
		reserve(size);
		for (size_type i = 0; i < size; ++i)
			copy_move_traits<T>::copy_n(*get(i), *o.get(i), 1);
		
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::operator=(PacketBuffer&& o)
	{
		assert(copy_move_traits::move_assignable);
		clear();
		head = std::move(o.head);
		sizePacketList = std::move(o.sizePacketList);
		capacityPacketList = std::move(o.capacityPacketList);
		o.head = nullptr;
		o.sizePacketList = 0;
		o.capacityPacketList = 0;
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	inline T& PacketBuffer<packetSize, T, allocator_t>::operator[](const size_type index)
	{
		size_type list = index / packetSize;
		size_type packet = index % packetSize;
		if (capacityPacketList < list + 1)
		{
			ReservePacketList(list + capacityPacketList);
			reserve(list);
		}
		return (*head[list])[packet];
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	inline PacketBuffer<packetSize, T, allocator_t>::PacketBuffer(const PacketBuffer& o)
	{
		*this = o;
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline PacketBuffer<packetSize, T, allocator_t>::PacketBuffer(PacketBuffer&& o)
	{
		*this = std::move(o);
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline PacketBuffer<packetSize, T, allocator_t>::~PacketBuffer()
	{
		clear();
		allocator_traits_list::deallocate(packetListAllocator, head);
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	inline void PacketBuffer<packetSize, T, allocator_t>::ReservePacketList(size_type size)
	{
		if (size < capacityPacketList)
			return;

		Packet** ptr = allocator_traits_list::allocate(packetListAllocator, size);
		if (head)
			memcpy_s(ptr, size * sizeof(Packet**), head, capacityPacketList * sizeof(Packet**));
		allocator_traits_list::deallocate(packetListAllocator, head);
		head = ptr;
		capacityPacketList = size;
	}

	template<size_type packetSize, class T, template<class>class allocator_t>
	template<class ...args>
	inline void PacketBuffer<packetSize, T, allocator_t>::constructs(const size_type listIndex, args ...arg)
	{
		construct_traits::constructs(constructor, head[listIndex]->value, packetSize, arg...);
	}

	template<size_type packetSize, class T, template<class> class allocator_t>
	inline size_type PacketBuffer<packetSize, T, allocator_t>::size() const
	{
		return sizePacketList * packetSize;
	}
	template<class T, int N>
	inline void BTreeNode< T, N>::insert(const T& value)
	{
		if (leaf)
		{
			InsertNode(value);
		}
		else
		{
			int idx = index(value);
			if (child[idx]->numKeys == n)
			{
				idx = split(idx);
				if (keys[idx] < value)
					idx++;
			}
			child[idx]->insert(value);
			size++;
		}
	}
	template<class T, int N>
	inline bool BTreeNode<T, N>::remove(const T& value)
	{
		int i = 0;
		int t = n / 2;
		while (i < numKeys && keys[i] < value)
			++i;

		if (i < numKeys && keys[i] == value)
		{
			if (leaf)
				RemoveFromLeaf(i);
			else
				RemoveFromNonLeaf(i);
			return true;
		}
		else
		{
			bool result = false;

			if (leaf)
				return false;

			if (child[i]->numKeys < t)
				fill(i);

			if (i == numKeys && i > numKeys)
				result = child[i - 1]->remove(value);
			else
				result = child[i]->remove(value);
			if (result)
				size--;
			return result;
		}
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::clear()
	{
		for (int i = 0; i < numKeys; ++i)
			if (child[i])
			{
				child[i]->clear();
				delete child[i];
			}
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::count_equal(const T& min, const T& max,int& c)const
	{
		int i = 0;
		while (i < numKeys && min > keys[i])
			++i;

		while (!leaf && i < numKeys && max >= keys[i])
		{
			child[i]->count_equal(min, max,c);
			i++;
		}
		for (int j = 0; j < numKeys; ++j)
		{
			if (min <= keys[j] && keys[j] <= max)
			{
				++c;
			}
		}
		if (!leaf && child[i])
			child[i]->count_equal(min, max, c);
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::count(const T& min, const T& max, int& c)const
	{
		int i = 0;
		while (i < numKeys && min > keys[i])
			++i;

		while (!leaf && i < numKeys && max >= keys[i])
		{
			child[i]->count(min, max, c);
			i++;
		}
		for (int j = 0; j < numKeys; ++j)
		{
			if (min < keys[j] && keys[j] < max)
			{
				++c;
			}
		}
		if (!leaf && child[i])
			child[i]->count(min, max, c);
	}

	template<class T, int N>
	inline int BTreeNode<T, N>::split(int idx)
	{
		this_t* newNode = new this_t();
		this_t* childNode = nullptr;
		int index = idx;
		int t = n / 2;

		childNode = child[index];
		childNode->size -= t + 1;
		newNode->size += t;

		for (int i = 0; i < t; ++i)
		{
			newNode->keys[i] = std::move(childNode->keys[i + t + 1]);
		}
		for (int i = 0; i < t + 1; ++i)
		{
			newNode->child[i] = childNode->child[i + t + 1];
			if (childNode->child[i + t + 1])
			{
				childNode->size -= childNode->child[i + t + 1]->size;
				newNode->size += childNode->child[i + t + 1]->size;
			}
			childNode->child[i + t + 1] = nullptr;
		}
		childNode->numKeys = t;
		newNode->numKeys = t;
		newNode->leaf = childNode->leaf;
		if (childNode->leaf)
		{
			childNode->size = t;
			newNode->size = t;
		}

		for (int i = n - 1; i > index; --i)
			child[i + 1] = child[i];
		child[index + 1] = newNode;

		InsertNode(childNode->keys[t]);
		return index;
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::traverse(int& counter,int buffer, T dst[])
	{
		int i = 0;
		for (i = 0; i < numKeys; ++i)
		{
			if (!leaf && child)
				child[i]->traverse(counter, buffer, dst);
			if (counter < buffer)
			{
				dst[counter] = keys[i];
				counter++;
			}
		}

		if (!leaf && child[i])
			child[i]->traverse(counter, buffer, dst);
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::traverse(int& counter, const T& min, const T& max, int buffer, T dst[])const
	{
		int i = 0;
		while (i < numKeys && min > keys[i])
			++i;
		
		while (!leaf && i < numKeys && max >= keys[i])
		{
			child[i]->traverse(counter, min, max, buffer, dst);
			i++;
		}
		for (int j = 0; j < numKeys; ++j)
		{
			if (min <= keys[j] && keys[j] <= max && counter < buffer)
			{
				dst[counter] = keys[j];
				counter++;
			}
		}
		if (!leaf && child[i])
			child[i]->traverse(counter, min, max, buffer, dst);
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::traverse(const T& min, const T& max, void(f)(T&))
	{
		int i = 0;
		while (i < numKeys && min > keys[i])
			++i;

		while (!leaf && i < numKeys && max >= keys[i])
		{
			child[i]->traverse(min, max, f);
			i++;
		}
		for (int j = 0; j < numKeys; ++j)
		{
			if (min <= keys[j] && keys[j] <= max)
			{
				f(keys[j]);
			}
		}
		if (!leaf && child[i])
			child[i]->traverse(min, max, f);

	}

	template<class T, int N>
	inline const typename BTreeNode<T,N>::this_t* BTreeNode<T, N>::find(const T& value)const
	{
		int i = 0;
		while (i < numKeys && value > keys[i])
			i++;

		// If the found key is equal to k, return this node
		if (i < numKeys && keys[i] == value)
			return this;

		// If key is not found here and this is a leaf node
		if (leaf == true)
			return nullptr;

		// Go to the appropriate child
		return child[i]->find(value);
	}

	template<class T, int N>
	inline const typename BTreeNode<T, N>::this_t* BTreeNode<T, N>::find(int& idx, const T& value)const
	{
		int i = 0;
		while (i < numKeys && value > keys[i])
			i++;

		// If the found key is equal to k, return this node
		if (i < numKeys && keys[i] == value)
		{
			idx = i;
			return this;
		}
		// If key is not found here and this is a leaf node
		if (leaf == true)
			return nullptr;

		// Go to the appropriate child
		return child[i]->find(idx,value);
	}

	template<class T, int N>
	inline typename BTreeNode<T, N>::this_t* BTreeNode<T, N>::find(const T& value)
	{
		int i = 0;
		while (i < numKeys && value > keys[i])
			i++;

		// If the found key is equal to k, return this node
		if (i < numKeys && keys[i] == value)
			return this;

		// If key is not found here and this is a leaf node
		if (leaf == true)
			return nullptr;

		// Go to the appropriate child
		return child[i]->find(value);
	}

	template<class T, int N>
	inline typename BTreeNode<T, N>::this_t* BTreeNode<T, N>::find(int& idx, const T& value)
	{
		int i = 0;
		while (i < numKeys && value > keys[i])
			i++;

		// If the found key is equal to k, return this node
		if (i < numKeys && keys[i] == value)
		{
			idx = i;
			return this;
		}
		// If key is not found here and this is a leaf node
		if (leaf == true)
			return nullptr;

		// Go to the appropriate child
		return child[i]->find(idx, value);
	}

	template<class T, int N>
	inline T& BTreeNode<T, N>::operator[](int idx)
	{
		assert(idx < this->size);
		int i = 0;
		int size = child[i] ? child[i]->size : 0;
		if (leaf)
			return keys[idx];
		while (size < idx && i < numKeys)
		{
			i++;
			size += child[i]->size + 1;
		}
		if (size == idx)
			return keys[i];

		return (*child[i])[idx - (size - child[i]->size)];

	}

	template<class T, int N>
	inline const T& BTreeNode<T, N>::operator[](int idx) const
	{
		assert(idx < this->size);
		int i = 0;
		int size = child[i] ? child[i]->size : 0;
		if (leaf)
			return keys[idx];
		while (size < idx && i < numKeys)
		{
			i++;
			size += child[i]->size + 1;
		}
		if (size == idx)
			return keys[i];

		return (*child[i])[idx - (size - child[i]->size)];

	}

	template<class T, int N>
	inline void BTreeNode<T, N>::operator=(const this_t& o)
	{
		numKeys = o.numKeys;
		size = o.size;
		leaf = o.leaf;
		for (int i = 0; i < numKeys; ++i)
			keys[i] = o.keys[i];
		for (int i = 0; i < n; ++i)
		{
			auto preNode = child[i];
			if (o.child[i])
			{
				child[i] = new this_t;
				*child[i] = *o.child[i];
			}
			if (preNode)
				delete preNode;
		}
	}

	template<class T, int N>
	inline void BTreeNode<T, N>::operator=(this_t&& o)
	{
		numKeys = o.numKeys;
		size = o.size;
		leaf = o.leaf;
		for (int i = 0; i < numKeys; ++i)
			keys[n] = o.keys[i];
		for (int i = 0; i < n; ++i)
		{
			auto preNode = child[i];
			if (o.child[i])
			{
				child[i] = new this_t;
				*child[i] = *o.child[i];
			}
		}
	}


	template<class T, int N>
	inline void BTreeNode<T, N>::InsertNode(const T& value)
	{
		int i = numKeys - 1;

		while (i >= 0 && keys[i] > value)
		{
			keys[i + 1] = keys[i];
			i--;
		}

		// Insert the new key at found location
		keys[i + 1] = value;
		numKeys++;
		if (leaf)
			size = numKeys;
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::fill(int idx)
	{
		int t = n / 2;
		if (idx != 0 && child[idx - 1]->numKeys >= t)
			BorrowFromPrev(idx);
		else if (idx != numKeys && child[idx + 1]->numKeys >= t)
			BorrowFromNext(idx);
		else
			if (idx != n)
				marge(idx);
			else
				marge(idx - 1);
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::BorrowFromPrev(int idx)
	{
		BTreeNode* child = this->child[idx]; 
		BTreeNode* sibling = this->child[idx - 1];

		for (int i = child->n - 1; i >= 0; --i)
			child->keys[i + 1] = child->keys[i];

		if (!child->leaf)
			for (int i = child->numKeys; i >= 0; --i)
				child->child[i + 1] = child->child[i];

		child->keys[0] = keys[idx - 1];

		if (!child->leaf)
		{
			child->child[0] = sibling->child[sibling->numKeys];
			child->size += sibling->child[sibling->numKeys]->size;
			sibling->size -= sibling->child[sibling->numKeys]->size;
		}

		keys[idx - 1] = sibling->keys[sibling->numKeys - 1];

		child->numKeys += 1;
		child->size += 1;
		sibling->numKeys -= 1;
		sibling->size -= 1;
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::BorrowFromNext(int idx)
	{
		BTreeNode* child = this->child[idx];
		BTreeNode* sibling = this->child[idx + 1];

		child->keys[(child->numKeys)] = keys[idx];


		if (!(child->leaf))
		{
			child->child[(child->numKeys) + 1] = sibling->child[0];
			child->size += sibling->child[0]->size;
			sibling->size -= sibling->child[0]->size;
		}

		keys[idx] = sibling->keys[0];

		for (int i = 1; i < sibling->numKeys; ++i)
			sibling->keys[i - 1] = sibling->keys[i];

		if (!sibling->leaf)
			for (int i = 1; i <= sibling->numKeys; ++i)
			{
				sibling->child[i - 1] = sibling->child[i];
				sibling->child[i] = nullptr;
			}
		child->numKeys += 1;
		child->size += 1;
		sibling->numKeys -= 1;
		sibling->size -= 1;
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::RemoveFromLeaf(int idx)
	{
		for (int i = idx + 1; i < numKeys; ++i)
			keys[i - 1] = keys[i];
		numKeys--;
		size--;
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::RemoveFromNonLeaf(int idx)
	{
		int t = n / 2;
		auto& k = keys[idx];

		if (child[idx]->numKeys >= t)
		{
			auto& pred = GetPred(idx);
			keys[idx] = pred;
			child[idx]->remove(pred);
		}
		else if (child[idx + 1]->numKeys >= t)
		{
			auto& succ = GetSucc(idx);
			keys[idx] = succ;
			child[idx + 1]->remove(succ);
		}
		else
		{
			marge(idx);
			child[idx]->remove(k);
		}
		--size;
	}
	template<class T, int N>
	inline T& BTreeNode<T, N>::GetPred(int idx)
	{
		auto cur = child[idx];
		while (!cur->leaf)
			cur = cur->child[cur->numKeys];
		return cur->keys[cur->numKeys - 1];
	}
	template<class T, int N>
	inline T& BTreeNode<T, N>::GetSucc(int idx)
	{
		auto cur = child[idx + 1];
		while (!cur->leaf)
			cur = cur->child[0];
		return cur->keys[0];
	}
	template<class T, int N>
	inline void BTreeNode<T, N>::marge(int idx)
	{
		int t = n / 2;
		BTreeNode* child = this->child[idx];
		BTreeNode* sibling = this->child[idx + 1];
		child->keys[t - 1] = keys[idx];

		for (int i = 0; i < sibling->numKeys; ++i)
		{
			child->keys[i + t] = sibling->keys[i];
		}
		if(!child->leaf)
			for (int i = 0; i <= sibling->numKeys; ++i)
			{
				child->child[i + t] = sibling->child[i];
				child->size += sibling->child[i]->size;
				sibling->size -= sibling->child[i]->size;
			}

		for (int i = idx + 1; i < numKeys; ++i)
			keys[i - 1] = keys[i];

		for (int i = idx + 2; i <= numKeys; ++i)
		{
			this->child[i - 1] = this->child[i];
			this->child[i] = nullptr;
		}
		child->numKeys += sibling->numKeys + 1;
		child->size += sibling->numKeys + 1;
		numKeys--;

		delete sibling;
	}
	template<class T, int N>
	inline int BTreeNode<T, N>::index(const T& value)
	{
		int i = numKeys - 1;
		while (i >= 0 && keys[i] > value)
			i--;
		return i + 1;
	}
	template<class T, int n>
	inline const typename BTree<T,n>::node_t* BTree<T, n>::find(const T& value)const
	{
		if (!size())
			return nullptr;
		return root->find(value);
	}
	template<class T, int n>
	inline const typename BTree<T, n>::node_t* BTree<T, n>::find(int& i, const T& value)const
	{
		if (!size())
			return nullptr;
		return root->find(i, value);
	}
	template<class T, int n>
	inline typename BTree<T, n>::node_t* BTree<T, n>::find(const T& value)
	{
		if (!size())
			return nullptr;
		return root->find(value);
	}
	template<class T, int n>
	inline typename BTree<T, n>::node_t* BTree<T, n>::find(int& i, const T& value)
	{
		if (!size())
			return nullptr;
		return root->find(i, value);
	}
	template<class T, int n>
	inline void BTree<T, n>::insert(const T& value)
	{
		if (root->numKeys == root->n)
		{
			node_t* s = new node_t;
			int size = root->size;
			s->child[0] = root;
			s->leaf = false;
			s->split(0);
			s->size = size;
			int i = 0;
			if (s->keys[0] < value)
				i++;
			s->child[i]->insert(value);
			s->size++;
			root = s;
		}
		else
			root->insert(value);
	}
	template<class T, int n>
	inline void BTree<T, n>::remove(const T& value)
	{
		if (root->size == 0)
			return;

		root->remove(value);
		if (root->numKeys == 0)
		{
			auto temp = root;
			if (!root->leaf)
				root = root->child[0];
			else
			{
				root = new node_t();
				root->leaf = true;
			}
			delete temp;
		}
	}
	template<class T, int n>
	inline void BTree<T, n>::traverse(int buffer, T dst[])
	{
		int counter = 0;
		root->traverse(counter, buffer, dst);
	}

	template<class T, int n>
	inline void BTree<T, n>::traverse(const T& min, const T& max, int buffer, T dst[])const
	{
		int counter = 0;
		root->traverse(counter, min, max, buffer, dst);
	}

	template<class T, int n>
	inline void BTree<T, n>::traverse(const T& min, const T& max, void(f)(T&))
	{
		root->traverse(min, max, f);
	}

	template<class T, int n>
	inline int BTree<T, n>::count_equal(const T& min, const T& max)const
	{
		int ret = 0;
		root->count_equal(min, max, ret);
		return ret;
	}

	template<class T, int n>
	inline int BTree<T, n>::count(const T& min, const T& max)const
	{
		int ret = 0;
		root->count(min, max, ret);
		return ret;
	}

	template<class T, int n>
	inline int BTree<T, n>::size() const
	{
		return root->size;
	}

	template<class T, int n>
	inline void BTree<T, n>::clear()
	{
		root->clear();
		root->numKeys = 0;
		root->size = 0;
		root->leaf = true;
		for (int i = 0; i < root->n + 1; ++i)
			root->child[i] = nullptr;
	}

	template<class T, int n>
	inline T& BTree<T, n>::operator[](int i)
	{
		return (*root)[i];
	}

	template<class T, int n>
	inline const T& BTree<T, n>::operator[](int i)const
	{
		return (*root)[i];
	}

	template<class T, int n>
	inline void BTree<T, n>::operator=(const BTree& o)
	{
		clear();
		*root = *o.root;
	}

	template<class T, int n>
	inline void BTree<T, n>::operator=(BTree&& o)
	{
		root = o.root;
		o.root = nullptr;
	}

	template<class T, int n>
	inline BTree<T, n>::BTree()
	{
		root = new node_t;
		root->leaf = true;
	}
	template<class T, int n>
	inline BTree<T, n>::BTree(const BTree& o)
	{
		*this = o;
	}
	template<class T, int n>
	inline BTree<T, n>::BTree(BTree&& o)
	{
		*this = std::move(o);
	}
	template<class T, int n>
	inline BTree<T, n>::~BTree()
	{
		clear();
	}
}