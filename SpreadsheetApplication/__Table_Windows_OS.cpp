#include "stdafx.h"

#ifdef _WINDOWS
#include "Utilities.h"
#include "__Table.h"
#include <WinUser.h>
#ifdef _WIN64
// GWL_WNDPROC -> GWLP_WNDPROC
#endif // _WIN64

using namespace std;
using namespace RYANS_UTILITIES;

const unsigned long id_Table_Background{ 1001 }, id_Text_Edit_Bar{ 1002 };

// Initial window setup
void WINDOWS_TABLE::InitializeTable() noexcept {
	hParent = hwnd;

	hTable = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Table_Background), hInst, NULL);		// Invisible background canvas window
	h_Text_Edit_Bar = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hParent, reinterpret_cast<HMENU>(id_Text_Edit_Bar), hInst, NULL);	// Upper edit box for input/edit of large strings
	auto hNull = CreateWindow(TEXT("static"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, hTable, reinterpret_cast<HMENU>(0), hInst, NULL);		// Empty label at position R 0, C 0

	// EditHandler stores the default window proceedure for all edit boxes. It only needs to be stored once.
	// For now, I am just grabbing it once for reuse later.
	// I expect to subclass the entry bar as well, but I don't have anything built for that yet.
	// In the meantime, I am just setting the proceedure back to what it was once I grabbed the default.
	EditHandler = reinterpret_cast<WNDPROC>(SetWindowLong(h_Text_Edit_Bar, GWLP_WNDPROC, reinterpret_cast<LONG>(EntryBarWindowProc)));
	//SetWindowLong(h_Text_Edit_Bar, GWL_WNDPROC, (LONG)EditHandler);

	Resize();		// Resize command will fill out the space with cell windows
	
	// Insert cells upon creation (Optional)
	CreateNewCell({ 1, 1 }, "2"s);
	CreateNewCell({ 2, 1 }, "4"s);
	CreateNewCell({ 3, 1 }, "9"s);
	CreateNewCell({ 4, 1 }, "SUM->"s);
	CreateNewCell({ 5, 1 }, "=SUM( &R1C1, &R1C2, &R1C3 )"s);

	CreateNewCell({ 1, 2 }, "=RECIPROCAL( &R1C1 )"s);
	CreateNewCell({ 2, 2 }, "=INVERSE( &R1C2 )"s);
	CreateNewCell({ 3, 2 }, "=&R1C3"s);
	CreateNewCell({ 4, 2 }, "SUM->"s);
	CreateNewCell({ 5, 2 }, "=SUM( &R2C1, &R2C2, &R2C3 )"s);

	CreateNewCell({ 1, 3 }, "&R2C1"s);
	CreateNewCell({ 2, 3 }, "&R2C2"s);
	CreateNewCell({ 3, 3 }, "&R2C3"s);
	CreateNewCell({ 4, 3 }, "AVERAGE->"s);
	CreateNewCell({ 5, 3 }, "=AVERAGE( &R3C1, &R3C2, &R3C3 )"s);

	auto startingCell = WINDOWS_TABLE::CELL_ID{ CELL::CELL_POSITION{ 1, 1 } };
	SetFocus(startingCell);	// Pick an arbitrary starting cell to avoid error where a cell is created with no position by clicking straight into upper entry bar.
}

// Logic for Cell Windows to construct and display the appropriate CELL based upon user input string
CELL::CELL_PROXY WINDOWS_TABLE::CreateNewCell(CELL::CELL_POSITION pos, std::string rawInput) const noexcept {
	auto oldCell = CELL::GetCellProxy(pos);
	auto nCell = CELL::cell_factory.NewCell(pos, rawInput);
	auto oldText = string{ };
	!oldCell ? oldText = ""s : oldText = oldCell->DisplayRawContent();
	if (nCell && rawInput != oldText) { undoStack.emplace_back(oldCell, nCell); redoStack.clear(); }
	return nCell;
}

WINDOWS_TABLE::~WINDOWS_TABLE() { }		// Hook for any on-exit logic

// Create a new row of cells at bottom edge.
void WINDOWS_TABLE::AddRow() noexcept {
	auto tempVec = vector<HWND>{ };
	auto cell_ID = WINDOWS_TABLE::CELL_ID{ };
	cell_ID.SetRow(++numRows).SetColumn(0);
	//cell_ID.SetRow(origin.row + ++numRows);

	// Create label for row
	auto label = wstring{ L"R" } + to_wstring(numRows);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0, y0 + (numRows)* height, width, height, hTable, cell_ID, hInst, NULL);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.GetColumn() < numColumns) {
		cell_ID.IncrementColumn();
		h = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + (numRows)* height, width, height, hTable, cell_ID, hInst, NULL);
		table->UpdateCell(cell_ID);								// Update cell display
		SetWindowLong(h, GWLP_WNDPROC, reinterpret_cast<LONG>(CellWindowProc));	// Associate with cell window procedure
	}
}

// Create a new column of cells at right edge.
void WINDOWS_TABLE::AddColumn() noexcept {
	auto cell_ID = WINDOWS_TABLE::CELL_ID{ };
	cell_ID.SetRow(0).SetColumn(++numColumns);
	//cell_ID.SetColumn(origin.column + ++numColumns);

	// Create label for column
	auto label = wstring{ L"C" } + to_wstring(numColumns);
	auto h = CreateWindow(TEXT("Static"), label.c_str(), WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + numColumns * width, y0, width, height, hTable, cell_ID, hInst, NULL);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.GetRow() < numRows) {
		cell_ID.IncrementRow();
		auto h = CreateWindow(TEXT("edit"), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, x0 + cell_ID.GetColumn() * width, y0 + cell_ID.GetRow() * height, width, height, hTable, cell_ID, hInst, NULL);
		table->UpdateCell(cell_ID);								// Update cell display
		SetWindowLong(h, GWLP_WNDPROC, reinterpret_cast<LONG>(CellWindowProc));	// Associate with cell window procedure
	}
}

// Remove bottom row of cells.
void WINDOWS_TABLE::RemoveRow() noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ };
	auto h = HWND{ id.SetRow(numRows) };
	while (id.GetColumn() <= numColumns) { DestroyWindow(h); h = id.IncrementColumn(); }	// Increment through row and destroy cells
	numRows--;
}

// Remove right column of cells.
void WINDOWS_TABLE::RemoveColumn() noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ };
	auto h = HWND{ id.SetRow(numRows) };
	while (id.GetRow() <= numRows) { DestroyWindow(h); h = id.IncrementRow(); }	// Increment through column and destroy cells
	numColumns--;
}

// Resize table, creating/destroying cell windows as needed.
void WINDOWS_TABLE::Resize() noexcept {
	auto rect = RECT{ };
	GetClientRect(hParent, &rect);		// Get size of window

	auto col = static_cast<unsigned int>(ceil(static_cast<double>(rect.right) / width) - 1);		// Leave off label column
	auto row = static_cast<unsigned int>(ceil(static_cast<double>(rect.bottom) / height) - 2);		// Leave off label row and Entry box

	// Place hard-cap on row & column number here.
	if (col > MaxColumn_) { col = MaxColumn_ - 1; }
	else if (col < 1) { col = 1; }
	if (row > MaxRow_) { row = MaxRow_ - 1; }
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
	auto cell = CELL::GetCellProxy(position);
	auto id = WINDOWS_TABLE::CELL_ID{ position };
	auto text = wstring{ };
	!cell ? text = L""s : text = string_to_wstring(cell->DisplayOutput());
	SetWindowText(id, text.c_str());
}

// Redraw table.
void WINDOWS_TABLE::Redraw() const noexcept {
	/*auto id = WINDOWS_TABLE::CELL_ID(origin);

	while (id.GetColumn() < numColumns) {
		id.SetColumn(id.GetColumn() + 1);
		SetWindowText();
	}*/
}

void WINDOWS_TABLE::FocusCell(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	auto cell = CELL::GetCellProxy(pos);
	auto text = wstring{ };
	mostRecentCell = pos;
	if (pos == posTargetCell) {							// If returning focus from upper-entry box...
		ReleaseTargetCell();							// Release focus when selecting straight to target to open it up for new target selection.
		text = Edit_Box_to_Wstring(h_Text_Edit_Bar);	// Text for cell creation comes from upper entry bar
		CreateNewCell(pos, wstring_to_string(text));	// Create new cell
		SetFocus(id);									// Set focus to self to avoid odd errors with changing focus to next cell
	}
	else if (cell) {										// If cell exists
		text = string_to_wstring(cell->DisplayRawContent());// Grab display text of cell
		SetWindowText(id, text.c_str());					// Show raw content rather than display value when cell is selected for editing.
		SendMessage(id, EM_SETSEL, 0, -1);					// Select all within cell.
		SetWindowText(h_Text_Edit_Bar, text.c_str());		// Otherwise, display raw content in entry bar.
	}
}

void WINDOWS_TABLE::UnfocusCell(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	auto text = Edit_Box_to_Wstring(id);
	CreateNewCell(pos, wstring_to_string(text));	// Create new cell
}

void WINDOWS_TABLE::FocusEntryBox() noexcept {
	if (posTargetCell != CELL::CELL_POSITION{ }) { return; }	// If there is a target, don't reset text
	LockTargetCell(mostRecentCell);								// Lock onto target cell
	auto id = WINDOWS_TABLE::CELL_ID{ mostRecentCell };
	auto text = Edit_Box_to_Wstring(id);
	SetWindowText(h_Text_Edit_Bar, text.c_str());				// Set text to that of target cell to continue editing
}

void WINDOWS_TABLE::UnfocusEntryBox(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	if (posTargetCell != CELL::CELL_POSITION{ } && pos != posTargetCell) {
		auto out = wstring{ L"&" };
		out += L"R" + to_wstring(id.GetRow());
		out += L"C" + to_wstring(id.GetColumn());
		SetFocus(h_Text_Edit_Bar);																// Set focus back to upper edit bar
		SendMessage(h_Text_Edit_Bar, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(out.c_str()));	// Get row and column of selected cell and add reference text to upper edit bar
	}
}

void WINDOWS_TABLE::FocusUp1(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	if (id.GetRow() == 1) { return; }			// Stop at bottom edge
	SetFocus(id.DecrementRow());				// Decrement row and set focus to new cell
}

void WINDOWS_TABLE::FocusDown1(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	if (id.GetRow() >= table->GetNumRows()) { /*winTable->origin.column++; winTable->Resize();*/ return; }		// Stop at top edge
	SetFocus(id.IncrementRow());	// Increment row and set focus to new cell
}

void WINDOWS_TABLE::FocusRight1(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	if (id.GetColumn() >= table->GetNumColumns()) { /*winTable->origin.column++; winTable->Resize();*/ return; }	// Stop at right edge
	SetFocus(id.IncrementColumn());				// Increment column and set focus to new cell
}

void WINDOWS_TABLE::FocusLeft1(CELL::CELL_POSITION pos) noexcept {
	auto id = WINDOWS_TABLE::CELL_ID{ pos };
	if (id.GetColumn() == 1) { return; }		// Stop at left edge
	SetFocus(id.DecrementColumn());				// Decrement column and set focus to new cell
}

void WINDOWS_TABLE::LockTargetCell(CELL::CELL_POSITION pos) noexcept { if (posTargetCell != CELL::CELL_POSITION{ }) { return; } posTargetCell = pos; }

void WINDOWS_TABLE::ReleaseTargetCell() noexcept { posTargetCell = CELL::CELL_POSITION{ }; }

CELL::CELL_POSITION WINDOWS_TABLE::TargetCellGet() const noexcept { return posTargetCell; }

void WINDOWS_TABLE::Undo() const noexcept {
	if (undoStack.empty()) { return; }
	auto& cell = undoStack.back().first;
	auto pos = undoStack.back().second->GetPosition();
	CELL::cell_factory.RecreateCell(cell, pos);			// Null cells need their position
	redoStack.push_back(undoStack.back());
	undoStack.pop_back();
}

void WINDOWS_TABLE::Redo() const noexcept {
	if (redoStack.empty()) { return; }
	auto cell = redoStack.back().second;
	CELL::cell_factory.RecreateCell(cell, cell->GetPosition());
	undoStack.push_back(redoStack.back());
	redoStack.pop_back();
}

#endif // WIN32