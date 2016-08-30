//-----------------------------------------------------------------------------
// ImageLoader.cpp
//
// Created by seriousviking at 2016.08.30 11:00
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "ImageLoader.h"

#include <dxgi1_5.h>
#include <d3d12.h>
#include <wincodec.h>

#include "Utils/Log.h"
#include "Utils/Macro.h"

namespace
{
	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
	{
		if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
		else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

		else return DXGI_FORMAT_UNKNOWN;
	}

	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
	{
		if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
		else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
		else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
		else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
		else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

		else return GUID_WICPixelFormatDontCare;
	}

	// get the number of bits per pixel for a dxgi format
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
	{
		if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
		else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
		else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
		else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
		else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
		else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
		else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

		else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
		else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
		else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
		else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
		else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
		else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
		else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
		else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;

		return 0;
	}
}

ImageLoader::ImageLoader() :
	_wicFactory(nullptr)
{
}

ImageLoader::~ImageLoader()
{
	clear();
}

void ImageLoader::clear()
{
	a_SAFE_RELEASE(_wicFactory);
}

SharedPtr<Image> ImageLoader::loadImageFromFile(const WString &path, const ConvertOptions &options)
{
	HRESULT result;
	SharedPtr<Image> image;

	if (!initWICFactory())
	{
		Log(L"ImageLoader::loadImageFromFile(): Failed to init WIC factory");
		return image;
	}
	assert(_wicFactory);

	IWICBitmapDecoder *wicDecoder = nullptr;
	IWICBitmapFrameDecode *wicFrame = nullptr;
	IWICFormatConverter *wicConverter = nullptr;

	do 
	{
		// load a decoder for the image
		result = _wicFactory->CreateDecoderFromFilename(
			path.c_str(),                     // Image we want to load in
			NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
			GENERIC_READ,                    // We want to read from this file
			WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
			&wicDecoder                      // the wic decoder to be created
		);
		if (FAILED(result))
		{
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to create decoder: %08x", path.c_str(), result);
			break;
		}

		UINT frameCount = 0;
		wicDecoder->GetFrameCount(&frameCount);
		if (FAILED(result))
		{
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to get frame count: %08x", path.c_str(), result);
			break;
		}
		if (frameCount == 0)
		{
			Log(L"ImageLoader::loadImageFromFile(%s): No frames found", path.c_str());
			break;
		}
		// get image from decoder (this will decode the "frame")
		result = wicDecoder->GetFrame(0, &wicFrame);
		if (FAILED(result))
		{
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to get frame 0: %08x", path.c_str(), result);
			break;
		}
		// get wic pixel format of image
		WICPixelFormatGUID pixelFormat;
		result = wicFrame->GetPixelFormat(&pixelFormat);
		if (FAILED(result)) 
		{ 
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to get pixel format: %08x", path.c_str(), result);
			break;
		}
		// get size of image
		UINT imageWidth, imageHeight;
		result = wicFrame->GetSize(&imageWidth, &imageHeight);
		if (FAILED(result)) 
		{
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to get image size: %08x", path.c_str(), result);
			break;
		}
		// TODO: add sRGB images support

		// convert wic pixel format to dxgi pixel format
		auto dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

		bool imageConverted = false;
		// if the format of the image is not a supported dxgi format, try to convert it
		if (dxgiFormat == DXGI_FORMAT_UNKNOWN || options.forceFormat != ConvertFormat::None)
		{
			WICPixelFormatGUID convertToPixelFormat = GUID_WICPixelFormatDontCare;
			// get a dxgi compatible wic format from the current image format
			switch (options.forceFormat)
			{
			case ConvertFormat::None:
			case ConvertFormat::Compatible:
				convertToPixelFormat = GetConvertToWICFormat(pixelFormat);
				break;
			case ConvertFormat::ToR32F:
				convertToPixelFormat = GUID_WICPixelFormat32bppGrayFloat;
				break;
			case ConvertFormat::ToR8UN:
				convertToPixelFormat = GUID_WICPixelFormat8bppGray;
				break;
			case ConvertFormat::ToRGBA8UN:
				convertToPixelFormat = GUID_WICPixelFormat32bppRGBA;
				break;
			default:
				Log(L"ImageLoader::loadImageFromFile(%s): Unsupported force convert format", path.c_str());
				break;
			}
			// return if no dxgi compatible format was found
			if (convertToPixelFormat == GUID_WICPixelFormatDontCare)
			{
				Log(L"ImageLoader::loadImageFromFile(%s): Unsupported WIC pixel format", path.c_str());
				break;
			}
			// set the dxgi format
			dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);
			// create the format converter
			result = _wicFactory->CreateFormatConverter(&wicConverter);
			if (FAILED(result)) 
			{
				Log(L"ImageLoader::loadImageFromFile(%s): Failed to create format converter: %08x", path.c_str(), result);
				break;
			}
			// make sure we can convert to the dxgi compatible format
			BOOL canConvert = FALSE;
			result = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
			if (FAILED(result) || !canConvert)
			{
				Log(L"ImageLoader::loadImageFromFile(%s): WIC cannot convert image: %08x", path.c_str(), result);
				break;
			}
			// do the conversion (wicConverter will contain the converted image)
			result = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
			if (FAILED(result))
			{
				Log(L"ImageLoader::loadImageFromFile(%s): WIC cannot initialize converter: %08x", path.c_str(), result);
				break;
			}
			// this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
			imageConverted = true;
		}
		int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
		UINT bytesPerRow = (imageWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
		int imageSize = bytesPerRow * imageHeight; // total image size in bytes
		// allocate enough memory for the raw image data, and set imageData to point to that memory
		BYTE* imageData = (BYTE*)malloc(imageSize);

		// copy (decoded) raw image data into the newly allocated memory (imageData)
		if (imageConverted)
		{
			// if image format needed to be converted, the wic converter will contain the converted image
			result = wicConverter->CopyPixels(0, bytesPerRow, imageSize, imageData);
		}
		else
		{
			// no need to convert, just copy data from the wic frame
			result = wicFrame->CopyPixels(0, bytesPerRow, imageSize, imageData);
		}
		if (FAILED(result))
		{
			Log(L"ImageLoader::loadImageFromFile(%s): Failed to copy image pixels: %08x", path.c_str(), result);
			break;
		}
		image = std::make_shared<Image>(imageWidth, imageHeight, bitsPerPixel / 8, bytesPerRow, dxgiFormat, imageData);
	} while (false);
	a_SAFE_RELEASE(wicDecoder);
	a_SAFE_RELEASE(wicFrame);
	a_SAFE_RELEASE(wicDecoder);
	return image;
}

SharedPtr<Image> ImageLoader::loadImageFromMemory(const Utils::FileData &data,
	const ConvertOptions &options)
{
	// TODO: implement loading from memory for better caching
	assert(false);//not implemented
	return SharedPtr<Image>();
}

bool ImageLoader::initWICFactory()
{
	HRESULT result;
	if (!_wicFactory)
	{
		// Initialize the COM library
		CoInitialize(NULL);

		// create the WIC factory
		result = CoCreateInstance(CLSID_WICImagingFactory, NULL,
			CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_wicFactory));
		if (FAILED(result))
		{
			Log(L"Failed to create WIC Factory: %08x", result);
			return false;
		}
	}
	return true;
}
