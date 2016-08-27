//-----------------------------------------------------------------------------
// FileUtils.h
//
// Created by seriousviking at 2016.08.26 14:01
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once
#include "Types.h"

namespace Utils
{
	typedef Vector<char> FileData;

	FileData loadFile(const WString &path);
}
