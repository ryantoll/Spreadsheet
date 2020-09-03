#include "stdafx.h"

#ifdef _WINDOWS
#include "Utilities.h"
#include "__Table.h"
#include "__WINDOW.h"

using namespace std;
using namespace WINDOWS_GUI;
using namespace RYANS_UTILITIES;

constexpr auto addExampleCells{ true };		// Add initial batch of example cells
const unsigned long id_Table_Background{ 1001 }, id_Text_Edit_Bar{ 1002 };

// Initial window setup
void WINDOWS_TABLE::InitializeTable() noexcept {
	hParent = hwnd;
	m_Table = ConstructChildWindow("static"s, hParent, id_Table_Background, hInst);				// Invisible background canvas window
	m_Text_Edit_Bar = ConstructChildWindow("edit"s, hParent, id_Text_Edit_Bar, hInst);			// Upper edit box for input/edit of large strings
	auto hNull = ConstructChildWindow("static"s, m_Table, reinterpret_cast<HMENU>(0), hInst);	// Empty label at position R 0, C 0
	EditHandler = m_Text_Edit_Bar.Procedure(EntryBarWindowProc);								// Save the default edit-box procedure for future use
	Resize();																					// Resize command will fill out the space with cell windows
	
	// Insert cells upon creation (Optional)
	if (addExampleCells) {
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
	}

	auto startingCell = CELL_ID{ CELL::CELL_POSITION{ 1, 1 } };
	SetFocus(startingCell);		// Pick an arbitrary starting cell to avoid error where a cell is created with no position by clicking straight into upper entry bar.
}

// Logic for Cell Windows to construct and display the appropriate CELL based upon user input string
CELL::CELL_PROXY WINDOWS_TABLE::CreateNewCell(const CELL::CELL_POSITION pos, const string& rawInput) const noexcept {
	auto oldCell = cellData.GetCellProxy(pos);
	auto nCell = CELL::NewCell(&cellData, pos, rawInput);
	auto oldText = string{ };
	!oldCell ? oldText = ""s : oldText = oldCell->GetRawContent();
	if (rawInput != oldText) { undoStack.emplace_back(oldCell, nCell); redoStack.clear(); }
	return nCell;
}

WINDOWS_TABLE::~WINDOWS_TABLE() { }		// Hook for any on-exit logic

void WINDOWS_TABLE::Redraw() const noexcept { }		// Hook for table redraw logic

// Create a new row of cells at bottom edge.
void WINDOWS_TABLE::AddRow() noexcept {
	auto cell_ID = CELL_ID{ }.Row(++numRows).Column(0);

	// Create label for row
	auto label = "R"s + to_string(numRows);
	auto window = ConstructChildWindow("Static"s, m_Table, cell_ID, hInst);
	auto pos = WINDOW_POSITION{ }.X(x0).Y(y0 + (numRows)*height);
	auto size = WINDOW_DIMENSIONS{ }.Height(height).Width(width);
	window.Text(label).Move(pos, size);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.Column() < numColumns) {
		cell_ID.IncrementColumn();
		window = ConstructChildWindow("edit"s, m_Table, cell_ID, hInst);
		pos.X(x0 + cell_ID.Column() * width).Y(y0 + (numRows)*height);
		window.Move(pos, size);
		window.Procedure(CellWindowProc);
		table->UpdateCell(cell_ID);
	}
}

// Create a new column of cells at right edge.
void WINDOWS_TABLE::AddColumn() noexcept {
	auto cell_ID = CELL_ID{ }.Row(0).Column(++numColumns);

	// Create label for column
	auto label = "C"s + to_string(numColumns);
	auto window = ConstructChildWindow("Static"s, m_Table, cell_ID, hInst);
	auto pos = WINDOW_POSITION{ }.X(x0 + numColumns * width).Y(y0);
	auto size = WINDOW_DIMENSIONS{ }.Height(height).Width(width);
	window.Text(label).Move(pos, size);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.Row() < numRows) {
		cell_ID.IncrementRow();
		window = ConstructChildWindow("edit"s, m_Table, cell_ID, hInst);
		pos.X(x0 + cell_ID.Column() * width).Y(y0 + cell_ID.Row() * height);
		window.Move(pos, size);
		window.Procedure(CellWindowProc);
		table->UpdateCell(cell_ID);
	}
}

// Remove bottom row of cells.
void WINDOWS_TABLE::RemoveRow() noexcept {
	auto id = CELL_ID{ };
	auto h = HWND{ id.Row(numRows) };
	while (id.Column() <= numColumns) { DestroyWindow(h); h = id.IncrementColumn(); }	// Increment through row and destroy cells
	numRows--;
}

// Remove right column of cells.
void WINDOWS_TABLE::RemoveColumn() noexcept {
	auto id = CELL_ID{ };
	auto h = HWND{ id.Row(numRows) };
	while (id.Row() <= numRows) { DestroyWindow(h); h = id.IncrementRow(); }	// Increment through column and destroy cells
	numColumns--;
}

// Resize table, creating/destroying cell windows as needed.
void WINDOWS_TABLE::Resize() noexcept {
	auto rect = WINDOW{ hParent }.GetClientRect();

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

	auto size = WINDOW_DIMENSIONS{ }.Width(rect.right).Height(rect.bottom);
	WINDOW{ m_Table }.Resize(size);								// Resize underlying canvas
	WINDOW{ m_Text_Edit_Bar }.Resize(size.Height(height));		// Resize upper entry box
}

void WINDOWS_TABLE::UpdateCell(const CELL::CELL_POSITION position) const noexcept {
	auto cell = cellData.GetCellProxy(position);
	auto text = string{ };
	!cell ? text = ""s : text = cell->GetOutput();
	WINDOW{ CELL_ID{ position } }.Text(text);
}

// Manage Focusing a cell in various contexts
void WINDOWS_TABLE::FocusCell(const CELL::CELL_POSITION pos) const noexcept {
	auto window = WINDOW{ CELL_ID{ pos } };
	auto cell = cellData.GetCellProxy(pos);
	auto text = string{ };
	mostRecentCell = pos;
	if (pos == posTargetCell) {							// If returning focus from upper-entry box...
		ReleaseTargetCell();							// Release focus when selecting straight to target to open it up for new target selection.
		CreateNewCell(pos, m_Text_Edit_Bar.Text());		// Create new cell from upper entry bar text
		window.Focus();									// Set focus to self to avoid odd errors with changing focus to next cell
	}
	else if (cell) {									// If cell exists
		text = cell->GetRawContent();					// Grab display text of cell
		window.Text(text);								// Show raw content rather than display value when cell is selected for editing
		window.Message(EM_SETSEL, 0, -1);				// Select all within cell
		m_Text_Edit_Bar.Text(text);						// Display raw content in entry bar
	}
	else { m_Text_Edit_Bar.Text(""s); }					// Otherwise, clear entry bar.
}

void WINDOWS_TABLE::UnfocusCell(const CELL::CELL_POSITION pos) const noexcept {
	CreateNewCell(pos, WINDOW{ CELL_ID{ pos } }.Text());
}

// Set text to that of target cell to continue editing
// If there is a target, don't reset text
void WINDOWS_TABLE::FocusEntryBox() const noexcept {
	if (posTargetCell != CELL::CELL_POSITION{ }) { return; }	
	LockTargetCell(mostRecentCell);
	auto text = WINDOW{ CELL_ID{ mostRecentCell } }.Text();
	m_Text_Edit_Bar.Text(text);
}

// If user clicks another cell while entering text, insert a reference to the selected cell
void WINDOWS_TABLE::UnfocusEntryBox(const CELL::CELL_POSITION pos) const noexcept {
	auto id = CELL_ID{ pos };
	if (posTargetCell != CELL::CELL_POSITION{ } && pos != posTargetCell) {
		auto out = wstring{ L"&" };
		out += L"R" + to_wstring(id.Row());
		out += L"C" + to_wstring(id.Column());
		m_Text_Edit_Bar.Focus();
		m_Text_Edit_Bar.Message(EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(out.c_str()));
	}
}

void WINDOWS_TABLE::FocusUp1(const CELL::CELL_POSITION pos) const noexcept {
	auto id = CELL_ID{ pos };
	if (id.Row() == 1) { return; }
	SetFocus(id.DecrementRow());
}

void WINDOWS_TABLE::FocusDown1(const CELL::CELL_POSITION pos) const noexcept {
	auto id = CELL_ID{ pos };
	if (id.Row() >= table->GetNumRows()) { return; }
	SetFocus(id.IncrementRow());
}

void WINDOWS_TABLE::FocusRight1(const CELL::CELL_POSITION pos) const noexcept {
	auto id = CELL_ID{ pos };
	if (id.Column() >= table->GetNumColumns()) { return; }
	SetFocus(id.IncrementColumn());
}

void WINDOWS_TABLE::FocusLeft1(const CELL::CELL_POSITION pos) const noexcept {
	auto id = CELL_ID{ pos };
	if (id.Column() == 1) { return; }
	SetFocus(id.DecrementColumn());
}

void WINDOWS_TABLE::LockTargetCell(const CELL::CELL_POSITION pos) const noexcept { if (posTargetCell != CELL::CELL_POSITION{ }) { return; } posTargetCell = pos; }

void WINDOWS_TABLE::ReleaseTargetCell() const noexcept { posTargetCell = CELL::CELL_POSITION{ }; }

CELL::CELL_POSITION WINDOWS_TABLE::TargetCellGet() const noexcept { return posTargetCell; }

// Transition: Cell B -> Cell A
// Move the transition pair {Cell A -> Cell B} onto redo stack
// If Cell A is NULL, then infer its position from Cell B
void WINDOWS_TABLE::Undo() const noexcept {
	if (undoStack.empty()) { return; }
	auto cell = undoStack.back().first;
	auto otherCell = undoStack.back().second;
	auto pos = CELL::CELL_POSITION{ };
	if (cell) { pos = cell->GetPosition(); }
	else { pos = otherCell->GetPosition(); }
	CELL::RecreateCell(&cellData, cell, pos);			// Null cells need their position
	redoStack.push_back(undoStack.back());
	undoStack.pop_back();
}

// Transition: Cell A -> Cell B
// Move the transition pair {Cell A -> Cell B} onto undo stack
// If Cell B is NULL, then infer its position from the Cell A
void WINDOWS_TABLE::Redo() const noexcept {
	if (redoStack.empty()) { return; }
	auto cell = redoStack.back().second;
	auto otherCell = redoStack.back().first;
	auto pos = CELL::CELL_POSITION{ };
	if (cell) { pos = cell->GetPosition(); }
	else { pos = otherCell->GetPosition(); }
	CELL::RecreateCell(&cellData, cell, pos);
	undoStack.push_back(redoStack.back());
	redoStack.pop_back();
}

#endif // WIN32