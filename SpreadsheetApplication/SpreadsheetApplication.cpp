/*///////////////////////////////////////////////////////////////////////////////////////////////
//This file processes the messages sent to the top-level window.
//Effectively, this translates window messages from the OS into appropriate function calls.
//This is an example of the "Bridge" pattern.
*////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SpreadsheetApplication.h"
#include "__Table.h"

LRESULT CALLBACK FrameWndProc(HWND hMain, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		//When the window is first opened...
	case WM_CREATE: {
		hwnd = hMain;						// hMain is also defined in Windows_Infrastructure, but this create command runs first. It needs to be defined before Setup_Window_Layout().
		table.reset(new WINDOWS_TABLE());
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
	case WM_SIZE : { table->Resize(); } break;
	//case WM_PAINT: { table.Resize(); } break;
	case WM_DESTROY: { PostQuitMessage(0); } break;
	default: { return DefWindowProc(hMain, message, wParam, lParam); }
	}
	return 0;
}