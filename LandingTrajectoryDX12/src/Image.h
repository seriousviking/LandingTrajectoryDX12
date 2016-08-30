//-----------------------------------------------------------------------------
// Image.h
//
// Created by seriousviking at 2016.08.30 15:25
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_5.h>

#include "Utils/Macro.h"

struct Image
{
	UINT width;
	UINT height;
	UINT bytesPerPixel;
	UINT bytesPerRow;
	DXGI_FORMAT format;
	BYTE* pixels;

	Image() :
		width(0),
		height(0),
		bytesPerPixel(0),
		bytesPerRow(0),
		format(DXGI_FORMAT_UNKNOWN),
		pixels(nullptr)
	{}
	Image(UINT newWidth, UINT newHeight, UINT newBytesPerPixel, 
		UINT newBytesPerRow, DXGI_FORMAT newFormat, BYTE* newPixels) :
		width(newWidth),
		height(newHeight),
		bytesPerPixel(newBytesPerPixel),
		bytesPerRow(newBytesPerRow),
		format(newFormat),
		pixels(newPixels)
	{}
	~Image()
	{
		a_SAFE_DELETE_ARRAY(pixels);
	}
};

