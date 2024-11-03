#include <stdio.h>
#include <string.h>
#include "DirectX/Jpeg.h"

extern "C"
{
#include "jpeglib.h"
}
#define ALIGNMENT_MUL 1.5

int Jpeg::JpegFileReadDecode(BitmapData* map, const char* fileName, SAMPLING_OPTION option)
{
	
	jpeg_decompress_struct jpeg;
	jpeg_error_mgr err;

	FILE* fi = nullptr;
	JSAMPLE* tmp;

	jpeg.err = jpeg_std_error(&err);
	fopen_s(&fi, fileName, "rb");
	if (fi == NULL)
	{
		return -1;
	}

	jpeg_create_decompress(&jpeg);
	jpeg_stdio_src(&jpeg, fi);
	jpeg_read_header(&jpeg, TRUE);
	jpeg_start_decompress(&jpeg);

	printf("width=%d,height=%d,ch=%d", jpeg.output_width, jpeg.output_height, jpeg.out_color_components);
	
	map->height = jpeg.output_height;
	map->width = jpeg.output_width;
	map->ch = jpeg.out_color_components;

	map->data = new unsigned char[map->width* jpeg.output_height * jpeg.out_color_components];
	if (map->data == NULL)
	{
		fclose(fi);
		delete[] map->data;
		return -1;
	}
	
	for (int i = 0; i < jpeg.output_height; ++i)
	{
		unsigned char* writePtr = GetWritePtrToOption(map,i,option);
		tmp = writePtr;
		jpeg_read_scanlines(&jpeg, &tmp, 1);
	}

	jpeg_finish_decompress(&jpeg);
	jpeg_destroy_decompress(&jpeg);
	fclose(fi);
	return 0;
}

int Jpeg::JpegMemReadDecode(BitmapData* map, const unsigned char* src, size_t size, SAMPLING_OPTION option)
{
	jpeg_decompress_struct jpeg;
	jpeg_error_mgr err;

	JSAMPLE* tmp;

	jpeg.err = jpeg_std_error(&err);

	jpeg_create_decompress(&jpeg);
	jpeg_mem_src(&jpeg, src, size);
	jpeg_read_header(&jpeg, TRUE);
	jpeg_start_decompress(&jpeg);

	printf("width=%d,height=%d,ch=%d", jpeg.output_width, jpeg.output_height, jpeg.out_color_components);

	map->height = jpeg.output_height;
	map->width = jpeg.output_width;
	map->ch = jpeg.out_color_components;

	map->data = new unsigned char[map->width * jpeg.output_height * jpeg.out_color_components];
	if (map->data == NULL)
	{
		delete[] map->data;
		return -1;
	}

	for (int i = 0; i < jpeg.output_height; ++i)
	{
		unsigned char* writePtr = GetWritePtrToOption(map, i, option);
		tmp = writePtr;
		jpeg_read_scanlines(&jpeg, &tmp, 1);
	}

	jpeg_finish_decompress(&jpeg);
	jpeg_destroy_decompress(&jpeg);
	return 0;
}

int Jpeg::JpegFileEncodeWrite(BitmapData* map, const char* fileName)
{
	jpeg_compress_struct jpeg;
	jpeg_error_mgr err;
	FILE* fo;
	JSAMPLE* address;
	
	jpeg.err = jpeg_std_error(&err);
	jpeg_create_compress(&jpeg);

	fopen_s(&fo, fileName, "wb");
	if (fo == NULL)
	{
		printf("%s‚ÍŠJ‚¯‚Ü‚¹‚ñ\n", fileName);
		jpeg_destroy_compress(&jpeg);
		return -1;
	}

	jpeg_stdio_dest(&jpeg, fo);

	jpeg.image_width = map->width;
	jpeg.image_height = map->height;
	jpeg.input_components = map->ch;
	jpeg.in_color_space = JCS_RGB;
	jpeg_set_defaults(&jpeg);
	jpeg_set_quality(&jpeg, 50, TRUE);

	jpeg_start_compress(&jpeg, TRUE);

	for (int i = 0; i < jpeg.image_height; ++i)
	{
		address = map->data + (i * map->width * map->ch);
		jpeg_write_scanlines(&jpeg, &address, 1);
	}

	jpeg_finish_compress(&jpeg);
	jpeg_destroy_compress(&jpeg);

	return 0;
}

Jpeg::BitmapData Jpeg::CopyBitmap(const BitmapData* data)
{
	BitmapData newBitmap = {};
	size_t dataSize = data->width * data->height * data->ch;
	newBitmap = *data;
	newBitmap.data = new unsigned char[dataSize];
	memcpy(newBitmap.data, data->data, dataSize);

	return newBitmap;
}

Jpeg::BitmapData Jpeg::ConvertToChannel(BitmapData* map, int ch)
{
	size_t newDataSize = map->width * map->height * ch;
	BitmapData newMap = {};
	if (ch > 4 || ch < 0)
		return BitmapData();

	newMap.data = new unsigned char[newDataSize] {};
	newMap.width = map->width;
	newMap.height = map->height;
	newMap.ch = ch;

	for (size_t i = 0; i < map->height; ++i)
	{
		for (size_t j = 0; j < map->width; ++j)
		{
			unsigned char* newMapPtr =
				newMap.data + i * map->width * ch + j * ch;
			unsigned char* srcMapPtr =
				map->data + i * map->width * map->ch + j * map->ch;

			int cpySize = ch < map->ch ? ch : map->ch;
			memcpy(newMapPtr, srcMapPtr, cpySize);
		}
	}

	return newMap;
}

int Jpeg::FreeBitMap(BitmapData* data)
{
	delete[] data->data;
	return 0;
}

unsigned char* Jpeg::GetWritePtrToOption(BitmapData* bitmap,size_t line, SAMPLING_OPTION option)
{
	switch (option)
	{
	case Jpeg::SAMPLING_OPTION_NONE:
		return bitmap->data + bitmap->width * line * bitmap->ch;
		break;
	case Jpeg::SAMPLING_OPTION_INV_X:
		break;
	case Jpeg::SAMPLING_OPTION_INV_Y:
		return bitmap->data + bitmap->width * ((bitmap->height-1) - line) * bitmap->ch;
		break;
	case Jpeg::SAMPLING_OPTION_INV_XY:
		break;
	default:
		break;
	}

	return nullptr;
}

void Jpeg::JpegDecompress(BitmapData* dst,  const unsigned char* src, unsigned char ch, size_t size, SAMPLING_OPTION option)
{
	Jpeg::BitmapData map;
	Jpeg::JpegMemReadDecode(&map, src, size, option);
	*dst = Jpeg::ConvertToChannel(&map, ch);
	FreeBitMap(&map);
}
