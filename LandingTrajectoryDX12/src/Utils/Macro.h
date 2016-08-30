//-----------------------------------------------------------------------------
// Macro.h
//
// Created by seriousviking at 2016.08.23 12:51
// see license details in LICENSE.md file
//-----------------------------------------------------------------------------
#pragma once

#define a_SAFE_RELEASE(x) { if (x) { x->Release(); x = nullptr; } }
#define a_SAFE_DELETE(x) { if (x) { delete x; x = nullptr; } }
#define a_SAFE_DELETE_ARRAY(x) { if (x) { delete[] x; x = nullptr; } }
