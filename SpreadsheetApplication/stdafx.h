#pragma once

#ifdef _WINDOWS

#include "targetver.h"
#include "Resource.h"
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>			// Windows Header Files
#include <WinUser.h>
extern HINSTANCE g_hInst;
extern HWND g_hWnd;
#undef min						// Windows macro that can collide with template<class T> std::min(T&, T&)
#endif // _WINDOWS

#ifdef _CONSOLE
#include <iostream>
#endif // _CONSOLE

#include <stdlib.h>				// C RunTime Header Files
#include <memory>

#include <algorithm>
#include <cassert>
#include <future>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>