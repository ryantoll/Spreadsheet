#pragma once

#ifdef WIN32

#include "targetver.h"
#include "Resource.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>					// Windows Header Files
extern HINSTANCE hInst;
extern HWND hwnd;

#endif // WIN32

#include <stdlib.h>						// C RunTime Header Files
#include <memory>

#include <algorithm>
#include <future>
#include <map>
#include <numeric>
#include <set>
#include <string>