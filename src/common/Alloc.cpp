#include "common/Alloc.h"
StackAllocator::StackAllocator(size_type stackSizeByte):
	byteSize(stackSizeByte),marker(0),stack((void*)(new char[stackSizeByte*STACK_BLOCK_SIZE]()))
{
}

void* StackAllocator::alloc(size_type sizeByte)
{
	assert(byteSize > marker + sizeByte);
	auto rptr = (void*)((Marker)stack + marker);
	marker += sizeByte * STACK_BLOCK_SIZE;
	return rptr;
}

StackAllocator::Marker StackAllocator::GetMarker()
{
	return marker;
}

void StackAllocator::FreeToMarker(Marker marker)
{
	this->marker -= marker * STACK_BLOCK_SIZE;
}

void StackAllocator::clear()
{
	marker = 0;
}

StackAllocator::~StackAllocator()
{
	delete[] stack;
}

void* AllocAligned(size_t bytes, size_t align)
{
	//割り当てるメモリの総量を求める
	size_t worstCaseBytes = bytes + align;
	//未アライメントブロックの割り当て
	uint8_t* pRawMem = new uint8_t[worstCaseBytes];

	//ブロックをアライメントする．もしアライメントシフトする
	//必要がない場合はアライメント値ぶんフルにシフトを行い
	//シフト量の格納用スペースを常に確保する
	uint8_t* pAlignedMem = AlignPointer(pRawMem, align);
	if (pAlignedMem == pRawMem)
		pAlignedMem += align;

	//シフト値を求め，格納する
	//最大で256バイトアライメントまで機能
	ptrdiff_t shift = pAlignedMem - pRawMem;
	assert(shift > 0 && shift <= 256);
	pAlignedMem[-1] = static_cast<uint8_t>(shift & 0xff);
	return pAlignedMem;
}

void FreeAligned(void* pMem)
{
	if (pMem)
	{
		//U8ポインタに変換
		uint8_t* pAlignedMem = reinterpret_cast<uint8_t*>(pMem);

		//シフト値を転回する
		ptrdiff_t shift = pAlignedMem[-1];
		if (shift == 0)
			shift = 256;

		//実際に割り当てられたアドレスを復元し
		//それを配列として削除する
		uint8_t* pRawMem = pAlignedMem - shift;
		delete[] pRawMem;
	}
}

void DoubleBufferAllocator::SwapBuffer()
{
	curStack = (uint32_t)!curStack;
}

void DoubleBufferAllocator::ClearCurrentBuffer()
{
	stack[curStack].clear();
}

void* DoubleBufferAllocator::alloc(uint32_t byteSize)
{
	return stack[curStack].alloc(byteSize);
}
