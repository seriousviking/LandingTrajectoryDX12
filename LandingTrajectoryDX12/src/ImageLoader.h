//-----------------------------------------------------------------------------
// ImageLoader.h
//
// Created by seriousviking at 2016.08.30 11:00
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
// code based on tutorial:
// http://www.braynzarsoft.net/viewtutorial/q16390-directx-12-textures-from-file
#pragma once
#include "Utils/Types.h"
#include "Utils/FileUtils.h"
#include "Image.h"

class ImageLoader
{
public:
	enum class ConvertFormat
	{
		None,      //No conversion
		Compatible,//Closest compatible format
		ToRGBA8UN, //DXGI_FORMAT_R8G8B8A8_UNORM
		ToR8UN,    //DXGI_FORMAT_R8_UNORM
		ToR32F,    //DXGI_FORMAT_R32_FLOAT
		// TODO: add more options if needed
	};

	struct ConvertOptions
	{
		ConvertFormat defaultFormat; // convert to this format if source format is unknown
		ConvertFormat forceFormat;   // force conversion to this format
		// TODO: add image scaling options
		// TODO: add multiframe image options
		ConvertOptions() : defaultFormat(ConvertFormat::ToRGBA8UN), forceFormat(ConvertFormat::None) {}
	};

	ImageLoader();
	ImageLoader(const ImageLoader &other) = delete;
	~ImageLoader();
	void clear();

	// TODO: add multi-frame image support (such as GIFs)
	SharedPtr<Image> loadImageFromFile(const WString &path, const ConvertOptions &options);
	SharedPtr<Image> loadImageFromMemory(const Utils::FileData &data, const ConvertOptions &options);

private:
	struct IWICImagingFactory *_wicFactory; // TODO: check if thread-safe

private:
	bool initWICFactory();
};
