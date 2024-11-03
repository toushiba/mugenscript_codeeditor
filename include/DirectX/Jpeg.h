#pragma once

namespace Jpeg
{
	struct BitmapData
	{
		unsigned char* data;
		unsigned int width;
		unsigned int height;
		unsigned int ch;
	};

	enum SAMPLING_OPTION
	{
		SAMPLING_OPTION_NONE,
		SAMPLING_OPTION_INV_X,
		SAMPLING_OPTION_INV_Y,
		SAMPLING_OPTION_INV_XY
	};

	inline size_t Aligned(size_t byteSize) { return (byteSize + 256) & 0xFFFFFF00; }
	int JpegFileReadDecode(BitmapData* map, const char* fileName,SAMPLING_OPTION option=SAMPLING_OPTION_NONE);
	int JpegMemReadDecode(BitmapData* map, const unsigned char* src, size_t size, SAMPLING_OPTION option = SAMPLING_OPTION_NONE);
	int JpegFileEncodeWrite(BitmapData* map, const char* fileName);
	BitmapData CopyBitmap(const BitmapData* src);
	BitmapData ConvertToChannel(BitmapData* map,int ch);
	int FreeBitMap(BitmapData*);
	unsigned char* GetWritePtrToOption(BitmapData* bitmap,size_t line,SAMPLING_OPTION option);
	void JpegDecompress(BitmapData* dst,const unsigned char* src,unsigned char ch,size_t size,SAMPLING_OPTION option=SAMPLING_OPTION_NONE);
}