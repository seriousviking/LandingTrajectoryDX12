//-----------------------------------------------------------------------------
// Log.cpp
//
// Created by seriousviking at 2016.08.22 21:12
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

WString s2ws(const String& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	Vector<wchar_t> buf(len);
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf.data(), len);
	WString r(buf.data());
	return r;
}

void PrintLog(const String &text)
{
#ifdef UNICODE
	WString ws = s2ws(text);
	OutputDebugString(ws.c_str());
#else
	OutputDebugString(text.c_str());
#endif
}

void PrintLog(const WString &text)
{
	OutputDebugStringW(text.c_str());
}
