/*
 * PngReader.h
 *
 *  Created on: 04.11.2010
 *      Author: Zahl
 */

#ifndef _PNGREADER_H_
#define _PNGREADER_H_

#include <stdint.h>
#include <cstdlib>

class PngReader
{
public:
	enum PngColorType {
		RGB,
		RGBA,
		GrayScale,
		GrayScaleAlpha,
		Palette,
		Unknown
	};

private:
	uint8_t *_imageData;
	uint32_t _width;
	uint32_t _height;
	PngColorType _chans;
	int _bitDepth;
	int _bytesPerPixel;

public:
	PngReader(const char* filename);
	virtual ~PngReader();
	uint32_t getWidth() { return _width; }
	uint32_t getHeight() { return _height; }
	PngColorType getColorType() { return _chans; }
	int getBitsPerChannel() { return _bitDepth; }
	bool isValidImage() { return _imageData != NULL; }
	uint8_t *getImageData() { return _imageData; }
	int getBytesPerPixel() { return _bytesPerPixel; }
};

#endif
