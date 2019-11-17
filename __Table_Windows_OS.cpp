#include "stdafx.h"
#include "__Table.h"
#include <WinUser.h>

const int id_Table_Background{ 32767 }, id_Text_Edit_Bar{ 32766 };
WNDPROC EditHandler;
HWND hParent, hTable, h_Text_Edit_Bar;
//vector<vector<HWND>> cellHandles;
map<int, HWND> cH;

void WINDOWS_TABLE::DrawTableOutline() {
	hParent = hwnd;

	hTable = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Table_Background), hInst, NULL);
	h_Text_Edit_Bar = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(id_Text_Edit_Bar), hInst, NULL);
	auto hNull = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(0), hInst, NULL);
	//cellHandles = { { hNull } };

	//EditHandler stores the default window proceedure for all edit boxes. It only needs to be stored once.
	//For now, I am just grabbing it once for reuse later.
	//I expect to subclass the entry bar as well, but I don't have anything built for that yet.
	//In the meantime, I am just setting the proceedure back to what it was once I grabbed the default.
	EditHandler = (WNDPROC)SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EditBoxProc);
	SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EditHandler);

	Resize();
}

//This window proceedure is what gives cells much of their UI functionality.
//Features include using arrow keys to move between cells and responding to user focus.
//Without this, cells would only display text on character input.
LRESULT CALLBACK EditBoxProc(HWND hEditBox, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message)
	{
	case WM_SETFOCUS: {
		SendMessage(hEditBox, EM_SETSEL, 0, -1);

		auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
		auto itCell = cellMap.find(id.GetCellPosition());
		if (itCell == cellMap.end()) { break; }											//Exit here if cell doesn't exist.
		SetWindowText(h_Text_Edit_Bar, itCell->second->DisplayRawContent().c_str());	//Otherwise, display raw content in entry bar.
		SendMessage(hEditBox, EM_SETSEL, 0, -1);
	} break;
	case WM_KILLFOCUS: {
		if (reinterpret_cast<HWND>(wParam) == h_Text_Edit_Bar) { break; }	//Do nothing if focus is switched to entry bar.

		//Tempory bug fix
		auto oldContent = Edit_Box_to_Wstring(h_Text_Edit_Bar);

		auto cellContents = Edit_Box_to_Wstring(hEditBox);
		SetWindowText(h_Text_Edit_Bar, L"");								//Clear entry bar.
		auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
		/*temporary bug fix*/	auto itCell = cellMap.find(id.GetCellPosition());
		/*temporary bug fix*/	if (cellContents != L"" && itCell != cellMap.end() && itCell->second->DisplayRawContent() == oldContent) { break; }
		auto cell = CELL::cell_factory.NewCell(id.GetCellPosition(), cellContents);
		if (!cell) { break; }												//Exit if no new cell is created.
		SetWindowText(hEditBox, cell->DisplayOutput().c_str());				//Otherwise, update cell content.
	} break;
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_RIGHT: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			auto winTable = dynamic_cast<WINDOWS_TABLE*>(table.get());
			if (id.GetColumn() >= winTable->numColumns) { /*winTable->origin.column++; winTable->Resize();*/ break; }
			int winID = id.SetColumn(id.GetColumn() + 1).GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_LEFT: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			if (id.GetColumn() == 1) { break; }
			int winID = id.SetColumn(id.GetColumn() - 1).GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_UP: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			if (id.GetRow() == 1) { break; }
			int winID = id.SetRow(id.GetRow() - 1).GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		case VK_DOWN: {
		} //vvvvvv__FallThrough__vvvvvv
		case VK_RETURN: {
			auto id = WINDOWS_TABLE::CELL_ID(GetDlgCtrlID(hEditBox));
			auto winTable = dynamic_cast<WINDOWS_TABLE*>(table.get());
			if (id.GetRow() >= winTable->numRows) { /*winTable->origin.column++; winTable->Resize();*/ break; }
			int winID = id.SetRow(id.GetRow() + 1).GetWindowID();
			auto h = GetDlgItem(hTable, id.GetWindowID());
			SetFocus(h);
		} break;
		}
	}
	break;
	}

	return CallWindowProc(EditHandler, hEditBox, message, wParam, lParam);
}

/*void WINDOWS_TABLE::AddRow() {
	vector<HWND> tempVec;
	WINDOWS_TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION() };
	cell_ID.SetRow(++numRows).SetColumn(0);

	wstring label = L"R" + to_wstring(numRows);
	tempVec.push_back(CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows) * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));

	wstring cellDisplay;
	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.SetColumn(cell_ID.GetColumn() + 1);
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		tempVec.push_back(CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows) * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));
		SetWindowLong(tempVec.back(), GWL_WNDPROC, (LONG)EditBoxProc);
	}

	cellHandles.push_back(tempVec);
}

void WINDOWS_TABLE::AddColumn() {
	WINDOWS_TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION() };
	cell_ID.SetRow(0).SetColumn(++numColumns);

	wstring label = L"C" + to_wstring(numColumns);
	cellHandles[cell_ID.GetRow()].push_back(CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + numColumns * width, y0, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));

	wstring cellDisplay;
	while (cell_ID.GetRow() < numRows) {
		cell_ID.SetRow(cell_ID.GetRow() + 1);
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		cellHandles[cell_ID.GetRow()].push_back(CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));
		SetWindowLong(cellHandles[cell_ID.GetRow()].back(), GWL_WNDPROC, (LONG)EditBoxProc);
	}
}

void WINDOWS_TABLE::RemoveRow() {
	auto& tempVec = cellHandles[numRows];
	for (auto h : tempVec) { DestroyWindow(h); }
	cellHandles.pop_back();
	numRows--;
}

void WINDOWS_TABLE::RemoveColumn() {
	for (auto vec: cellHandles) {
		DestroyWindow(vec[numColumns]);
		vec.pop_back();
	}
	numColumns--;
}*/

void WINDOWS_TABLE::AddRow() {
	vector<HWND> tempVec;
	WINDOWS_TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION{ } };
	cell_ID.SetRow(++numRows).SetColumn(0);
	//cell_ID.SetRow(origin.row + ++numRows);

	wstring label = L"R" + to_wstring(numRows);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);

	wstring cellDisplay;
	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.SetColumn(cell_ID.GetColumn() + 1);
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		h = CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows)* height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		SetWindowLong(h, GWL_WNDPROC, (LONG)EditBoxProc);
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
		cell_ID.SetRow(cell_ID.GetRow() + 1);
		auto itCell = cellMap.find(cell_ID.GetCellPosition());
		if (itCell != cellMap.end()) { cellDisplay = itCell->second->DisplayOutput(); }
		else { cellDisplay.clear(); }
		auto h = CreateWindow(TEXT("edit"), cellDisplay.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL);
		SetWindowLong(h, GWL_WNDPROC, (LONG)EditBoxProc);
	}
}

void WINDOWS_TABLE::RemoveRow() {
	auto id = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ } };
	id.SetRow(numRows);
	auto h = GetDlgItem(hTable, id.GetWindowID());

	while (id.GetColumn() <= numColumns) {
		DestroyWindow(h);
		id.SetColumn(id.GetColumn() + 1);
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
		id.SetRow(id.GetRow() + 1);
		h = GetDlgItem(hTable, id.GetWindowID());
	}

	numColumns--;
}

void WINDOWS_TABLE::Resize() {
	RECT rect;
	GetClientRect(hParent, &rect);

	int col = ceil(static_cast<double>(rect.right) / width) - 1;		//Leave off label column
	int row = ceil(static_cast<double>(rect.bottom) / height) - 2;		//Leave off label row and Entry box

	if (col > 255) { col = 255; }
	if (row > 255) { row = 255; }

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