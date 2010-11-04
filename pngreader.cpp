/*
 * PngReader.cpp
 *
 *  Created on: 04.11.2010
 *      Author: Zahl
 */

#include "pngreader.h"
#include <png.h>
#include <cstdio>

PngReader::PngReader(const char* filename)
{
	_width = _height = _bitDepth = 0;
	_chans = Unknown;
	_imageData = NULL;
	uint8_t **rows = NULL;
	// Open PNG file
	FILE *fh = fopen(filename, "rb");
	if (fh == NULL) {
		return;
	}
	png_byte header[8]; // Check header
	if (fread(header, 1, 8, fh) != 8 || png_sig_cmp(header, 0, 8)) {
		// Not a PNG file
		fclose(fh);
		return;
	}
	// Set up libpng to read file
	png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop pngInfo = NULL;
	if (pngPtr == NULL || setjmp(png_jmpbuf(pngPtr))) {
		png_destroy_read_struct(&pngPtr, &pngInfo, NULL);
		fclose(fh);
		delete[] rows;
		return;
	}
	pngInfo = png_create_info_struct(pngPtr);
	png_init_io(pngPtr, fh);
	png_set_sig_bytes(pngPtr, 8);
	png_read_info(pngPtr, pngInfo);
	png_uint_32 width, height;
	int type, interlace, comp, filter; // Check image format (square, RGBA)
	png_uint_32 ret = png_get_IHDR(pngPtr, pngInfo, &width, &height, &_bitDepth, &type, &interlace, &comp, &filter);
	if (ret == 0) {
		png_destroy_read_struct(&pngPtr, &pngInfo, NULL);
		fclose(fh);
		return;
	}
	// Assign properties
	_width = width;
	_height = height;
	int bytesPerPixel = 0;
	switch (type) {
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		_chans = GrayScaleAlpha;
		bytesPerPixel = 2;
		break;
	case PNG_COLOR_TYPE_GRAY:
		_chans = GrayScale;
		bytesPerPixel = 1;
		break;
	case PNG_COLOR_TYPE_PALETTE:
		_chans = Palette;
		bytesPerPixel = 1;
		break;
	case PNG_COLOR_TYPE_RGB:
		_chans = RGB;
		bytesPerPixel = 3;
		break;
	case PNG_COLOR_TYPE_RGBA:
		_chans = RGBA;
		bytesPerPixel = 4;
		break;
	default:
		_chans = Unknown;
	}
	bytesPerPixel = (bytesPerPixel * _bitDepth) / 8;
	// Alloc mem
	_imageData = new uint8_t[width * height * bytesPerPixel];
	rows = new uint8_t*[height];
	for (png_uint_32 i = 0; i < height; ++i) {
		rows[i] = _imageData + i * width * bytesPerPixel;
	}
	png_read_image(pngPtr, rows);
	png_destroy_read_struct(&pngPtr, &pngInfo, NULL);
	delete[] rows;
}

PngReader::~PngReader()
{
	delete[] _imageData;
}
