#include "stdafx.h"
#include "__Table.h"

const int id_Table_Background{ 32767 }, id_Text_Edit_Bar{ 32766 };

void TABLE::DrawTableOutline(HWND h) {
	hParent = h;

	hTable = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Table_Background), hInst, NULL);
	hEntryBar = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(id_Text_Edit_Bar), hInst, NULL);
	auto hNull = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(0), hInst, NULL);
	cellHandles = { { hNull } };

	Resize();
}

void TABLE::AddRow() {
	vector<HWND> tempVec;
	TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION() };
	cell_ID.SetRow(++numRows).SetColumn(0);

	wstring label = L"R" + to_wstring(numRows);
	tempVec.push_back(CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows) * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));

	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.SetColumn(cell_ID.GetColumn() + 1);
		tempVec.push_back(CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows) * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));
	}

	cellHandles.push_back(tempVec);
}

void TABLE::AddColumn() {
	TABLE::CELL_ID cell_ID{ CELL::CELL_POSITION() };
	cell_ID.SetRow(0).SetColumn(++numColumns);

	wstring label = L"C" + to_wstring(numColumns);
	cellHandles[cell_ID.GetRow()].push_back(CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + numColumns * width, y0, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));

	while (cell_ID.GetRow() < numRows) {
		cell_ID.SetRow(cell_ID.GetRow() + 1);
		cellHandles[cell_ID.GetRow()].push_back(CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, reinterpret_cast<HMENU>(cell_ID.GetWindowID()), hInst, NULL));
	}
}

void TABLE::RemoveRow() {
	auto& tempVec = cellHandles[numRows];
	for (auto h : tempVec) { DestroyWindow(h); }
	cellHandles.pop_back();
	numRows--;
}

void TABLE::RemoveColumn() {
	for (auto vec: cellHandles) {
		DestroyWindow(vec[numColumns]);
		vec.pop_back();
	}
	numColumns--;
}

void TABLE::Resize() {
	RECT rect;
	GetClientRect(hParent, &rect);

	int col = ceil(static_cast<double>(rect.right) / width);
	int row = ceil(static_cast<double>(rect.bottom) / height) - 1;

	if (col > 255) { col = 255; }
	if (row > 255) { row = 255; }

	while (numColumns < col) { AddColumn(); }
	while (numColumns > col) { RemoveColumn(); }
	while (numRows < row) { AddRow(); }
	while (numRows > row) { RemoveRow(); }

	MoveWindow(hTable, 0, 0, rect.right, rect.bottom, true);
	MoveWindow(hEntryBar, 0, 0, rect.right, height, true);
}