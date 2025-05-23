/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file processes the messages sent to the top-level window.
// Effectively, this translates window messages from the OS into appropriate TABLE function calls.
// This is a variant of the "Adapter" pattern, turning procedural message processing into an object interface.
// A preprocessor command was added to seperate out Windows-specific code.
// A similar scheme could be done for other systems to select the desired code with a simple flag.
*///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "framework.hpp"
#include "SpreadsheetApplication.hpp"
#include "Table_Windows_OS.hpp"

LRESULT CALLBACK FrameWndProc(HWND hMain, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
		// Just before window is fully created
	case WM_CREATE: {
		g_hWnd = hMain;	// hMain is also defined in Windows_Infrastructure, but this create command runs first. It needs to be defined before DrawTableOutline().
		table = std::make_unique<WINDOWS_TABLE>();
		table->InitializeTable();
	} break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId) {
			case ID_EDIT_UNDO: { table->Undo(); } break;
			case ID_EDIT_REDO: { table->Redo(); } break;
			case IDM_ABOUT: { DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hMain, About); } break;
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

// This window proceedure is what gives cells much of their UI functionality.
// Features include using arrow keys to move between cells and responding to user focus.
// Without this, cells would only display text on character input.
LRESULT CALLBACK CellWindowProc(HWND hEditBox, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_SETFOCUS: {	table->FocusCell(WINDOWS_TABLE::CELL_ID{ hEditBox }); } break;
	case WM_KILLFOCUS: { if (m_TextEditBar != reinterpret_cast<HWND>(wParam)) { table->UnfocusCell(WINDOWS_TABLE::CELL_ID{ hEditBox }); } } break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RIGHT: { table->FocusRight1(WINDOWS_TABLE::CELL_ID{ hEditBox }); } break;
		case VK_LEFT: {	table->FocusLeft1(WINDOWS_TABLE::CELL_ID{ hEditBox }); } break;
		case VK_UP: { table->FocusUp1(WINDOWS_TABLE::CELL_ID{ hEditBox }); } break;
		case VK_DOWN:	[[fallthrough]];
		case VK_RETURN: { table->FocusDown1(WINDOWS_TABLE::CELL_ID{ hEditBox }); } break;
		}
	} break;
	}

	return CallWindowProc(m_EditHandler, hEditBox, message, wParam, lParam);
}

// Custom behavior of upper entry box
LRESULT CALLBACK EntryBarWindowProc(HWND hEntryBox, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_SETFOCUS: { table->FocusEntryBox(); } break;
	case WM_KILLFOCUS: { auto pos = WINDOWS_TABLE::CELL_ID{ reinterpret_cast<HWND>(wParam) }; table->UnfocusEntryBox(pos); } break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RETURN: { table->FocusCell(table->TargetCellGet()); } break;	// Set focus back to target cell
		}
	} break;
	}

	return CallWindowProc(m_EditHandler, hEntryBox, message, wParam, lParam);
}
