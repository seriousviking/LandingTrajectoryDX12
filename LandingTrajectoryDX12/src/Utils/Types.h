//-----------------------------------------------------------------------------
// Types.h
//
// Created by seriousviking at 2016.08.21 19:06
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once

#include <vector>
#include <memory>
#include <type_traits>
#include <string>
#include <map>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <cstdio>

typedef unsigned char uChar;
typedef int           Int;
typedef unsigned int  uInt;
typedef float         Float;
typedef double        Double;

template<class T> using Vector = std::vector<T>;
template<class T> using SharedPtr = std::shared_ptr<T>;
template<class T> using WeakPtr = std::weak_ptr<T>;
template<class T> using UniquePtr = std::unique_ptr<T>;
using String = std::string;
using WString = std::wstring;
template<class Key, class Value> using Map = std::map<Key, Value>;
