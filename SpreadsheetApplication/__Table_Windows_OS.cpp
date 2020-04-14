#include "stdafx.h"
#include "__Table.h"
#include <WinUser.h>

const int id_Table_Background{ 32767 }, id_Text_Edit_Bar{ 32766 };
WNDPROC EditHandler;
HWND hParent, hTable, h_Text_Edit_Bar;
LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);

void WINDOWS_TABLE::DrawTableOutline() {
	hParent = hwnd;

	hTable = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Table_Background), hInst, NULL);
	h_Text_Edit_Bar = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(id_Text_Edit_Bar), hInst, NULL);
	auto hNull = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(0), hInst, NULL);

	// EditHandler stores the default window proceedure for all edit boxes. It only needs to be stored once.
	// For now, I am just grabbing it once for reuse later.
	// I expect to subclass the entry bar as well, but I don't have anything built for that yet.
	// In the meantime, I am just setting the proceedure back to what it was once I grabbed the default.
	EditHandler = (WNDPROC)SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EntryBarWindowProc);
	//SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EditHandler);

	Resize();
}

bool CreateNewCell(HWND hMostRecentCell, wstring rawInput) {
	///*temporary bug fix*/	auto oldContent = Edit_Box_to_Wstring(h_Text_Edit_Bar);
	SetWindowText(h_Text_Edit_Bar, L"");											// Clear entry bar.
	auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hMostRecentCell));
	///*temporary bug fix*/	auto itCell = cellMap.find(id.GetCellPosition());
	///*temporary bug fix*/	if (rawInput != L"" && itCell != cellMap.end() && itCell->second->DisplayRawContent() == oldContent) { return false; }
	auto cell = CELL::cell_factory.NewCell(id.GetCellPosition(), rawInput);
	if (!cell) { return false; }													// Exit if no new cell is created.
	SetWindowText(hMostRecentCell, cell->DisplayOutput().c_str());					// Otherwise, update cell content.
	return true;
}

// This window proceedure is what gives cells much of their UI functionality.
// Features include using arrow keys to move between cells and responding to user focus.
// Without this, cells would only display text on character input.
LRESULT CALLBACK CellWindowProc(HWND hEditBox, UINT message, WPARAM wParam, LPARAM lParam) {
	static auto hMostRecentCell = HWND{ };		// For switching between cells and upper input bar, track which cell is focused.
	switch (message)
	{
	case WM_SETFOCUS: {
		if (reinterpret_cast<HWND>(wParam) == h_Text_Edit_Bar && hMostRecentCell != hEditBox) {		// If coming in from upper input bar from another cell, create new cell.
			CreateNewCell(hMostRecentCell, Edit_Box_to_Wstring(h_Text_Edit_Bar));
		}

		auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
		auto itCell = cellMap.find(id.GetCellPosition());
		if (itCell == cellMap.end()) { break; }											// Exit here if cell doesn't exist.
		SetWindowText(h_Text_Edit_Bar, itCell->second->DisplayRawContent().c_str());	// Otherwise, display raw content in entry bar.
		SetWindowText(hEditBox, itCell->second->DisplayRawContent().c_str());			// Show raw content rather than display value when cell is selected for editing.
		SendMessage(hEditBox, EM_SETSEL, 0, -1);										// Select all within cell.
	} break;
	case WM_KILLFOCUS: {
		hMostRecentCell = hEditBox;
		if (reinterpret_cast<HWND>(wParam) == h_Text_Edit_Bar) { break; }				// Do nothing if focus is switched to entry bar.
		CreateNewCell(hEditBox, Edit_Box_to_Wstring(hEditBox));
			///*temporary bug fix*/	auto oldContent = Edit_Box_to_Wstring(h_Text_Edit_Bar);
		//auto cellContents = Edit_Box_to_Wstring(hEditBox);
		//SetWindowText(h_Text_Edit_Bar, L"");											// Clear entry bar.
		//auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			///*temporary bug fix*/	auto itCell = cellMap.find(id.GetCellPosition());
			///*temporary bug fix*/	if (cellContents != L"" && itCell != cellMap.end() && itCell->second->DisplayRawContent() == oldContent) { break; }
		//auto cell = CELL::cell_factory.NewCell(id.GetCellPosition(), cellContents);
		//if (!cell) { break; }															// Exit if no new cell is created.
		//SetWindowText(hEditBox, cell->DisplayOutput().c_str());							// Otherwise, update cell content.
	} break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RIGHT: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			auto winTable = dynamic_cast<WINDOWS_TABLE*>(table.get());
			if (id.GetColumn() >= winTable->numColumns) { /*winTable->origin.column++; winTable->Resize();*/ break; }
			int winID = id.IncrementColumn().GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_LEFT: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			if (id.GetColumn() == 1) { break; }
			int winID = id.DecrementColumn().GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_UP: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			if (id.GetRow() == 1) { break; }
			int winID = id.DecrementRow().GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_DOWN: {
		} // vvvvvv__FallThrough__vvvvvv
		case VK_RETURN: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			auto winTable = dynamic_cast<WINDOWS_TABLE*>(table.get());
			if (id.GetRow() >= winTable->numRows) { /*winTable->origin.column++; winTable->Resize();*/ break; }
			int winID = id.IncrementRow().GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		}
	}
	break;
	}

	return CallWindowProc(EditHandler, hEditBox, message, wParam, lParam);
}

LRESULT CALLBACK EntryBarWindowProc(HWND hEntryBox, UINT message, WPARAM wParam, LPARAM lParam) {
	static auto hMostRecentCell = HWND{ };
	switch (message)
	{
	case WM_SETFOCUS: {	hMostRecentCell = reinterpret_cast<HWND>(wParam); } break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RETURN: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hMostRecentCell));
			auto winTable = dynamic_cast<WINDOWS_TABLE*>(table.get());
			if (id.GetRow() >= winTable->numRows) { /*winTable->origin.column++; winTable->Resize();*/ break; }
			int winID = id.IncrementRow().GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		}
	} break;
	}

	return CallWindowProc(EditHandler, hEntryBox, message, wParam, lParam);
}

void WINDOWS_TABLE::AddRow() {
	vector<HWND> tempVec;
	WINDOWS_TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION{ } };
	cell_ID.SetRow(++numRows).SetColumn(0);
	//cell_ID.SetRow(origin.row + ++numRows);

	wstring label = L"R" + to_wstring(numRows);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);

	wstring cellDisplay;
	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.IncrementColumn();
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		h = CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		SetWindowLong(h, GWL_WNDPROC, (LONG)CellWindowProc);
	}
}

void WINDOWS_TABLE::AddColumn() {
	WINDOWS_TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION{ } };
	cell_ID.SetRow(0).SetColumn(++numColumns);
	//cell_ID.SetColumn(origin.column + ++numColumns);

	wstring label = L"C" + to_wstring(numColumns);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + numColumns * width, y0, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);

	wstring cellDisplay;
	while (cell_ID.GetRow() < numRows) {
		cell_ID.IncrementRow();
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		auto h = CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		SetWindowLong(h, GWL_WNDPROC, (LONG)CellWindowProc);
	}
}

void WINDOWS_TABLE::RemoveRow() {
	auto id = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	id.SetRow(numRows);
	auto h = GetDlgItem(hTable, id.GetWindowID());

	while (id.GetColumn() <= numColumns) {
		DestroyWindow(h);
		id.IncrementColumn();
		h = GetDlgItem(hTable, id.GetWindowID());
	}

	numRows--;
}

void WINDOWS_TABLE::RemoveColumn() {
	auto id = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	id.SetColumn(numColumns);
	auto h = GetDlgItem(hTable, id.GetWindowID());

	while (id.GetRow() <= numRows) {
		DestroyWindow(h);
		id.IncrementRow();
		h = GetDlgItem(hTable, id.GetWindowID());
	}

	numColumns--;
}

void WINDOWS_TABLE::Resize() {
	RECT rect;
	GetClientRect(hParent, &rect);

	int col = ceil(static_cast<double>(rect.right) / width) - 1;		// Leave off label column
	int row = ceil(static_cast<double>(rect.bottom) / height) - 2;		// Leave off label row and Entry box

	if (col > 255) { col = 255; }
	else if (col < 1) { col = 1; }
	if (row > 255) { row = 255; }
	else if (row < 1) { row = 1; }

	while (numColumns < col) { AddColumn(); }
	while (numColumns > col) { RemoveColumn(); }
	while (numRows < row) { AddRow(); }
	while (numRows > row) { RemoveRow(); }

	MoveWindow(hTable, 0, 0, rect.right, rect.bottom, true);
	MoveWindow(h_Text_Edit_Bar, 0, 0, rect.right, height, true);
}

void WINDOWS_TABLE::UpdateCell(CELL::CELL_POSITION position) {
	auto id = WINDOWS_TABLE::CELL_ID{ position };
	auto h = GetDlgItem(hTable, id.GetWindowID());
	auto out = cellMap[position]->DisplayOutput();
	SetWindowText(h, out.c_str());
}

void WINDOWS_TABLE::Redraw() {
	/*auto id = WINDOWS_TABLE::CELL_ID(origin);

	while (id.GetColumn() < numColumns) {
		id.SetColumn(id.GetColumn() + 1);
		SetWindowText();
	}*/
}