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
	//���蓖�Ă郁�����̑��ʂ����߂�
	size_t worstCaseBytes = bytes + align;
	//���A���C�����g�u���b�N�̊��蓖��
	uint8_t* pRawMem = new uint8_t[worstCaseBytes];

	//�u���b�N���A���C�����g����D�����A���C�����g�V�t�g����
	//�K�v���Ȃ��ꍇ�̓A���C�����g�l�Ԃ�t���ɃV�t�g���s��
	//�V�t�g�ʂ̊i�[�p�X�y�[�X����Ɋm�ۂ���
	uint8_t* pAlignedMem = AlignPointer(pRawMem, align);
	if (pAlignedMem == pRawMem)
		pAlignedMem += align;

	//�V�t�g�l�����߁C�i�[����
	//�ő��256�o�C�g�A���C�����g�܂ŋ@�\
	ptrdiff_t shift = pAlignedMem - pRawMem;
	assert(shift > 0 && shift <= 256);
	pAlignedMem[-1] = static_cast<uint8_t>(shift & 0xff);
	return pAlignedMem;
}

void FreeAligned(void* pMem)
{
	if (pMem)
	{
		//U8�|�C���^�ɕϊ�
		uint8_t* pAlignedMem = reinterpret_cast<uint8_t*>(pMem);

		//�V�t�g�l��]�񂷂�
		ptrdiff_t shift = pAlignedMem[-1];
		if (shift == 0)
			shift = 256;

		//���ۂɊ��蓖�Ă�ꂽ�A�h���X�𕜌���
		//�����z��Ƃ��č폜����
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
