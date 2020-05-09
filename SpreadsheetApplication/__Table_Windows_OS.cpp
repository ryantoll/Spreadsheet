#include "stdafx.h"
#include "Utilities.h"
#include "__Table.h"
#include <WinUser.h>

using std::wstring;
using std::to_wstring;
using std::vector;
using namespace RYANS_UTILITIES;

const unsigned long id_Table_Background{ 1001 }, id_Text_Edit_Bar{ 1002 };
WNDPROC EditHandler;
HWND hParent, hTable, h_Text_Edit_Bar;
LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);

// Initial window setup
void WINDOWS_TABLE::DrawTableOutline() noexcept {
	hParent = hwnd;

	hTable = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Table_Background), hInst, NULL);		// Invisible background canvas window
	h_Text_Edit_Bar = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Text_Edit_Bar), hInst, NULL);	// Upper edit box for input/edit of large strings
	auto hNull = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(0), hInst, NULL);		// Empty label at position R 0, C 0

	// EditHandler stores the default window proceedure for all edit boxes. It only needs to be stored once.
	// For now, I am just grabbing it once for reuse later.
	// I expect to subclass the entry bar as well, but I don't have anything built for that yet.
	// In the meantime, I am just setting the proceedure back to what it was once I grabbed the default.
	EditHandler = (WNDPROC)SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EntryBarWindowProc);
	//SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EditHandler);

	Resize();		// Resize command will fill out the space with cell windows
	auto startingCell = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ 1, 1 } };
	SetFocus(startingCell.GetWindowHandle());	// Pick an arbitrary starting cell to avoid error where a cell is created with no position by clicking straight into upper entry bar.
}

// Logic for Cell Windows to construct and display the appropriate CELL based upon user input string
bool CreateNewCell(HWND hMostRecentCell, wstring rawInput) noexcept {
	SetWindowText(h_Text_Edit_Bar, L"");														// Clear entry bar
	auto id = WINDOWS_TABLE::CELL_ID(hMostRecentCell);
	auto cell = CELL::cell_factory.NewCell(id.GetCellPosition(), wstring_to_string(rawInput));	// Create new cell
	return cell ? true : false;
}

// This window proceedure is what gives cells much of their UI functionality.
// Features include using arrow keys to move between cells and responding to user focus.
// Without this, cells would only display text on character input.
LRESULT CALLBACK CellWindowProc(HWND hEditBox, UINT message, WPARAM wParam, LPARAM lParam) {
	static auto hTargetCell = HWND{ };		// For switching between cells and upper input bar, track which cell is focused.
	switch (message)
	{
	case WM_SETFOCUS: {
		// If coming in from upper input box, set cell to show that text
		// Otherwise, show the raw text (if any) associated with that cell
		if (reinterpret_cast<HWND>(wParam) == h_Text_Edit_Bar && hTargetCell == hEditBox) {
			auto text = Edit_Box_to_Wstring(h_Text_Edit_Bar);
			SetWindowText(hEditBox, text.c_str());
			break;
		}

		if (hTargetCell == HWND{ }) { hTargetCell = HWND{ }; }					// If there is no target, this is now it.
		auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
		auto itCell = cellMap.find(id.GetCellPosition());
		if (itCell == cellMap.end()) { break; }									// Exit here if cell doesn't exist.
		auto out = string_to_wstring(itCell->second->DisplayRawContent());
		SetWindowText(h_Text_Edit_Bar, out.c_str());							// Otherwise, display raw content in entry bar.
		SetWindowText(hEditBox, out.c_str());									// Show raw content rather than display value when cell is selected for editing.
		SendMessage(hEditBox, EM_SETSEL, 0, -1);								// Select all within cell.
	} break;
	case WM_KILLFOCUS: {
		auto hComparison = reinterpret_cast<HWND>(wParam);
		if (hComparison != h_Text_Edit_Bar) { CreateNewCell(hEditBox, Edit_Box_to_Wstring(hEditBox)); hTargetCell = HWND{ }; break; }	// Create new CELL if not continuing editing in upper entry box
		else if (hTargetCell == HWND{ }) { hTargetCell = hEditBox; }	// Lock on to target cell
	} break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RIGHT: {
			auto id = WINDOWS_TABLE::CELL_ID(hEditBox);
			if (id.GetColumn() >= table->GetNumColumns()) { /*winTable->origin.column++; winTable->Resize();*/ break; }	// Stop at right edge
			id.IncrementColumn().GetWindowID();			// Increment cell ID
			SetFocus(id.GetWindowHandle());				// Set focus to new cell
		} break;
		case VK_LEFT: {
			auto id = WINDOWS_TABLE::CELL_ID(hEditBox);
			if (id.GetColumn() == 1) { break; }			// Stop at left edge
			id.DecrementColumn().GetWindowID();			// Decrement cell ID
			SetFocus(id.GetWindowHandle());				// Set focus to new cell
		} break;
		case VK_UP: {
			auto id = WINDOWS_TABLE::CELL_ID(hEditBox);
			if (id.GetRow() == 1) { break; }			// Stop at bottom edge
			id.DecrementRow().GetWindowID();			// Decrement cell ID
			SetFocus(id.GetWindowHandle());				// Set focus to new cell
		} break;
		case VK_DOWN: // vvvvvv__FallThrough__vvvvvv
		case VK_RETURN: {
			auto id = WINDOWS_TABLE::CELL_ID(hEditBox);
			if (id.GetRow() >= table->GetNumRows()) { /*winTable->origin.column++; winTable->Resize();*/ break; }		// Stop at top edge
			id.IncrementRow().GetWindowID();	// Increment cell ID
			SetFocus(id.GetWindowHandle());		// Set focus to new cell
		} break;
		}
	}
	break;
	}

	return CallWindowProc(EditHandler, hEditBox, message, wParam, lParam);
}

// Custom behavior of upper entry box
LRESULT CALLBACK EntryBarWindowProc(HWND hEntryBox, UINT message, WPARAM wParam, LPARAM lParam) {
	static auto hMostRecentCell = HWND{ }, hTargetCell = HWND{ };
	switch (message)
	{
	case WM_SETFOCUS: {		// Set window text to that of the most recent edit box with focus
		hMostRecentCell = reinterpret_cast<HWND>(wParam);
		if (hTargetCell != HWND{ }) { break; }			// If there is a target, don't reset text
		hTargetCell = hMostRecentCell;					// Lock onto target cell
		auto text = Edit_Box_to_Wstring(hTargetCell);	
		SetWindowText(hEntryBox, text.c_str());			// Set text to that of target cell to continue editing
	} break;
	case WM_KILLFOCUS: {
		if (hTargetCell == HWND{ }) { break; }							// Don't do anything if there is no target cell
		auto hNewCell = reinterpret_cast<HWND>(wParam);
		if (hNewCell == hTargetCell) { hTargetCell = HWND{ }; break; }	// Release focus when selecting straight to target to open it up for new target selection.
		auto id = WINDOWS_TABLE::CELL_ID(hNewCell);
		auto out = wstring{ L"&" };
		out += L"R" + to_wstring(id.GetRow());
		out += L"C" + to_wstring(id.GetColumn());
		SendMessage(hEntryBox, EM_REPLACESEL, 0, (LPARAM)out.c_str());	// Get row and column of selected cell and add reference text to upper edit bar
		SetFocus(hEntryBox);											// Set focus back to upper edit bar
	} break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RETURN: {
			auto id = WINDOWS_TABLE::CELL_ID(hTargetCell);
			SetFocus(hTargetCell);												// Set focus back to target cell
			//SendMessage(hTargetCell, WM_KEYDOWN, (WPARAM)VK_RETURN, NULL);	// Send "Return-key" message to target cell		// This message seems to get lost in the suffle  :/  User needs to press "return" again.
			hTargetCell = HWND{ };												// Release focus
		} break;
		}
	} break;
	}

	return CallWindowProc(EditHandler, hEntryBox, message, wParam, lParam);
}

WINDOWS_TABLE::~WINDOWS_TABLE() { }		// Hook for any on-exit logic

// Create a new row of cells at bottom edge.
void WINDOWS_TABLE::AddRow() noexcept {
	auto tempVec = vector<HWND>{ };
	auto cell_ID = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	cell_ID.SetRow(++numRows).SetColumn(0);
	//cell_ID.SetRow(origin.row + ++numRows);

	// Create label for row
	auto label = wstring{ L"R" } + to_wstring(numRows);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.IncrementColumn();
		h = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		table->UpdateCell(cell_ID.GetCellPosition());			// Update cell display
		SetWindowLong(h, GWL_WNDPROC, (LONG)CellWindowProc);	// Associate with cell window procedure
	}
}

// Create a new column of cells at right edge.
void WINDOWS_TABLE::AddColumn() noexcept {
	auto cell_ID = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	cell_ID.SetRow(0).SetColumn(++numColumns);
	//cell_ID.SetColumn(origin.column + ++numColumns);

	// Create label for column
	auto label = wstring{ L"C" } + to_wstring(numColumns);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + numColumns * width, y0, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.GetRow() < numRows) {
		cell_ID.IncrementRow();
		auto h = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		table->UpdateCell(cell_ID.GetCellPosition());			// Update cell display
		SetWindowLong(h, GWL_WNDPROC, (LONG)CellWindowProc);	// Associate with cell window procedure
	}
}

// Remove bottom row of cells.
void WINDOWS_TABLE::RemoveRow() noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	auto h = id.SetRow(numRows).GetWindowHandle();
	while (id.GetColumn() <= numColumns) { DestroyWindow(h); h = id.IncrementColumn().GetWindowHandle(); }	// Increment through row and destroy cells
	numRows--;
}

// Remove right column of cells.
void WINDOWS_TABLE::RemoveColumn() noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	auto h = id.SetRow(numRows).GetWindowHandle();
	while (id.GetRow() <= numRows) { DestroyWindow(h); h = id.IncrementRow().GetWindowHandle(); }	// Increment through column and destroy cells
	numColumns--;
}

// Resize table, creating/destroying cell windows as needed.
void WINDOWS_TABLE::Resize() noexcept {
	auto rect = RECT{ };
	GetClientRect(hParent, &rect);		// Get size of window

	auto col = static_cast<unsigned int>(ceil(static_cast<double>(rect.right) / width) - 1);		// Leave off label column
	auto row = static_cast<unsigned int>(ceil(static_cast<double>(rect.bottom) / height) - 2);		// Leave off label row and Entry box

	// Place hard-cap on row & column number here.
	if (col > 65535) { col = 65535; }
	else if (col < 1) { col = 1; }
	if (row > 65535) { row = 65535; }
	else if (row < 1) { row = 1; }

	// Add/remove rows and columns until it is exactly filled
	// Each run, at least two while loops will not trigger
	while (numColumns < col) { AddColumn(); }
	while (numColumns > col) { RemoveColumn(); }
	while (numRows < row) { AddRow(); }
	while (numRows > row) { RemoveRow(); }

	MoveWindow(hTable, 0, 0, rect.right, rect.bottom, true);		// Resize underlying canvas
	MoveWindow(h_Text_Edit_Bar, 0, 0, rect.right, height, true);	// Resize upper entry box
}

// Update cell text.
void WINDOWS_TABLE::UpdateCell(CELL::CELL_POSITION position) const noexcept {
	if (cellMap.find(position) == cellMap.end()) { return; }	// Return if no cell data exits
	auto id = WINDOWS_TABLE::CELL_ID{ position };
	auto out = string_to_wstring(cellMap[position]->DisplayOutput());
	SetWindowText(id.GetWindowHandle(), out.c_str());
}

// Redraw table.
void WINDOWS_TABLE::Redraw() const noexcept {
	/*auto id = WINDOWS_TABLE::CELL_ID(origin);

	while (id.GetColumn() < numColumns) {
		id.SetColumn(id.GetColumn() + 1);
		SetWindowText();
	}*/
}

HWND WINDOWS_TABLE::CELL_ID::GetWindowHandle() const noexcept { return GetDlgItem(hTable, windowID); }