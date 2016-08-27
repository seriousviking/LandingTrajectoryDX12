//-----------------------------------------------------------------------------
// FileUtils.cpp
//
// Created by seriousviking at 2016.08.26 14:00
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "FileUtils.h"

namespace Utils
{
	FileData loadFile(const WString &path)
	{

		FileData fileData;
		using namespace std;
		ifstream ifs;
		ifs.open(path, ios::binary | ios::ate);
		if (ifs.is_open())
		{
			size_t fileSize = ifs.tellg();
			ifs.seekg(0, ios::beg);
			fileData.resize(fileSize);
			if (!ifs.read(fileData.data(), fileData.size()))
			{
				fileData.clear();
			}
			ifs.close();
		}
		return fileData;
	}

}