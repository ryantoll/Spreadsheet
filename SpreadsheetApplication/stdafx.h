#pragma once

#include "targetver.h"
#include "Resource.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>					// Windows Header Files
#include <stdlib.h>						// C RunTime Header Files
#include <memory>

#include <map>
#include <string>
#include <algorithm>
#include <numeric>
#include <future>

extern HINSTANCE hInst;
extern HWND hwnd;