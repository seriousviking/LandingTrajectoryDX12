//-----------------------------------------------------------------------------
// Log.h
//
// Created by seriousviking at 2016.08.22 21:12
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once
#include "Types.h"

#if !defined(NDEBUG) || defined(_DEBUG)
#define DEBUG_LOG_ENABLED 1
#endif

void PrintLog(const String &text);
void PrintLog(const WString &text);

// based on answer http://stackoverflow.com/a/26221725 
// from http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
String stringFormatA(const String& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	return String(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template<typename ... Args>
WString stringFormatW(const WString& format, Args ... args)
{
	size_t size = wsnprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	unique_ptr<wchar_t[]> buf(new wchar_t[size]);
	wsnprintf(buf.get(), size, format.c_str(), args ...);
	return WString(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

#ifdef DEBUG_LOG_ENABLED
template <typename ... Args>
void Log(const String& format, Args ... args)
{
	String text = stringFormatA(format, args);
	PrintLog(text);
}

template <typename ... Args>
void Log(const WString& format, Args ... args)
{
	WString text = stringFormatA(format, args);
	PrintLog(text);
}

#else // DEBUG_LOG_ENABLED

inline int StubElepsisFunctionForLog(...) { return 0; }
static class StubClassForLog {
public:
	inline void operator =(size_t) {}
private:
	inline StubClassForLog &operator =(const StubClassForLog &)
	{
		return *this;
	}
} StubForLogObject;

#define Log \
    StubForLogObject = sizeof StubElepsisFunctionForLog
#endif // DEBUG_LOG_ENABLED


// took from Vulkan test project. Might be useful in the future
WString s2ws(const String& s);

