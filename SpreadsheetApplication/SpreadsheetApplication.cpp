/*///////////////////////////////////////////////////////////////////////////////////////////////
//This file processes the messages sent to the top-level window.
//Effectively, this translates window messages from the OS into appropriate TABLE function calls.
//This is part of a "Bridge" pattern performed by the WINDOWS_TABLE class.
*////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SpreadsheetApplication.h"
#include "__Table.h"

LRESULT CALLBACK FrameWndProc(HWND hMain, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
		// Just before window is fully created
	case WM_CREATE: {
		hwnd = hMain;						// hMain is also defined in Windows_Infrastructure, but this create command runs first. It needs to be defined before DrawTableOutline().
		table = std::make_unique<WINDOWS_TABLE>();
		table->DrawTableOutline();
	} break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
			case IDM_ABOUT: { DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hMain, About); } break;
			case IDM_EXIT: { DestroyWindow(hMain); } break;
			default: { return DefWindowProc(hMain, message, wParam, lParam); }
		}
	} break;
	case WM_SIZE : { table->Resize(); } break;			// Window changed in size. Often falls through to WM_PAINT command if such exists.
	//case WM_PAINT: { table.Resize(); } break;			// Special logic for drawing window
	case WM_DESTROY: { PostQuitMessage(0); } break;		// Add here any logic for program exit
	default: { return DefWindowProc(hMain, message, wParam, lParam); }
	}
	return 0;
}