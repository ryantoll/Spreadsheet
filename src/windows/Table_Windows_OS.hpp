#include "framework.hpp"

#include "Utilities.hpp"
#include "Table.hpp"
#include "WINDOW.hpp"

using namespace std;
using namespace RYANS_UTILITIES;
using namespace RYANS_UTILITIES::WINDOWS_GUI;

constexpr auto addExampleCells{ true };		// Add initial batch of example cells
const unsigned long id_Table_Background{ 1001 }, id_Text_Edit_Bar{ 1002 };

inline RYANS_UTILITIES::WINDOWS_GUI::WINDOW m_Table, m_TextEditBar;
inline WNDPROC m_EditHandler;
LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);

// Table class specialized for a Windows OS GUI
class WINDOWS_TABLE : public TABLE_BASE {
public:
	class CELL_ID;
	class CELL_WINDOW;
protected:
	unsigned int m_NumColumns{ 0 };
	unsigned int m_NumRows{ 0 };
	int m_Width{ 75 };
	int m_Height{ 25 };
	int m_X0{ 0 };
	int m_Y0{ 25 };
	HWND hParent{ nullptr };
	mutable CELL::CELL_DATA m_CellData;
	mutable CELL::CELL_POSITION m_Origin{ 0, 0 };		// Origin is the "off-the-begining" cell to the upper-left of the upper-left cell
	mutable CELL::CELL_POSITION m_PosTargetCell{ };	// Tracks position of cell currently associated with upper edit box, may be blank
	mutable CELL::CELL_POSITION m_MostRecentCell{ };	// Tracks position of most recently selected cell for either target selection or new cell creation

	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> m_UndoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> m_RedoStack{ };
public:
	~WINDOWS_TABLE();						// Hook for any on-exit logic
	void AddRow() override;
	void AddColumn() override;
	void RemoveRow() override;
	void RemoveColumn() override;
	unsigned int GetNumColumns() const override { return m_NumColumns; }
	unsigned int GetNumRows() const override { return m_NumRows; }

	void InitializeTable() override;
	void Resize() override;
	void Redraw() const override;

	void FocusCell(const CELL::CELL_POSITION) const override;
	void UnfocusCell(const CELL::CELL_POSITION) const override;
	void FocusEntryBox() const override;
	void UnfocusEntryBox(const CELL::CELL_POSITION) const override;

	void FocusUp1(const CELL::CELL_POSITION) const override;
	void FocusDown1(const CELL::CELL_POSITION) const override;
	void FocusRight1(const CELL::CELL_POSITION) const override;
	void FocusLeft1(const CELL::CELL_POSITION) const override;

	CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const;
	void UpdateCell(const CELL::CELL_POSITION) const override;
	void LockTargetCell(const CELL::CELL_POSITION) const override;
	void ReleaseTargetCell() const override;
	CELL::CELL_POSITION TargetCellGet() const override;

	void Undo() const override;
	void Redo() const override;
};

// This is a utility class for converting between window IDs and row/column indicies.
// The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
// The ID will correspond to the row and column number with 16 bits representing each.
// This is scoped within the WINDOWS_TABLE class to indicate to clients that it's intended usage is only within the context of WINDOWS_TABLE usage.
// Define constructors as 'explicit' to avoid any implicit conversions since they take exactly one argument.
class WINDOWS_TABLE::CELL_ID {
	CELL::CELL_POSITION position;
	unsigned long windowID{ 0 };
	void Win_ID_From_Position() { windowID = (position.column << 16) + position.row; }
	void Position_From_Win_ID() {
		position.column = windowID >> 16;
		position.row = windowID & UINT16_MAX;
	}
public:
	CELL_ID() = default;
	explicit CELL_ID(const CELL::CELL_POSITION newPosition) : position(newPosition) { Win_ID_From_Position(); }
	explicit CELL_ID(const unsigned long newWindowID) : windowID(newWindowID) { Position_From_Win_ID(); }
	explicit CELL_ID(const HWND h) : windowID(GetDlgCtrlID(h)) { Position_From_Win_ID(); }

	void WindowID(const int newID) { windowID = newID; Position_From_Win_ID(); }
	auto& Row(const unsigned int newRowIndex) { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& Column(const unsigned int newColumnIndex) { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }
	auto& SetCellPosition(const CELL::CELL_POSITION newPosition) { position = newPosition; Win_ID_From_Position(); return *this; }

	auto WindowID() const { return windowID; }
	auto Row() const { return position.row; }
	auto Column() const { return position.column; }

	auto& IncrementRow() { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() { position.column--; Win_ID_From_Position(); return *this; }

	operator CELL::CELL_POSITION() const { return position; }				// Allow for implicit conversion to CELL_POSITION
	operator HWND() const { return GetDlgItem(m_Table, windowID); }		// Allow for implicit conversion to HWND
	operator HMENU() const { return reinterpret_cast<HMENU>(static_cast<UINT_PTR>(windowID)); }	// Allow for implicit conversion to HMENU
};

// Initial window setup
void WINDOWS_TABLE::InitializeTable() {
	hParent = g_hWnd;
	m_Table = ConstructChildWindow("static"s, hParent, id_Table_Background);			// Invisible background canvas window
	m_TextEditBar = ConstructChildWindow("edit"s, hParent, id_Text_Edit_Bar);			// Upper edit box for input/edit of large strings
	auto hNull = ConstructChildWindow("static"s, m_Table, reinterpret_cast<HMENU>(0));	// Empty label at position R 0, C 0
	m_EditHandler = m_TextEditBar.Procedure(EntryBarWindowProc);						// Save the default edit-box procedure for future use
	Resize();																			// Resize command will fill out the space with cell windows
	
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
CELL::CELL_PROXY WINDOWS_TABLE::CreateNewCell(const CELL::CELL_POSITION pos, const string& rawInput) const {
	auto oldCell = m_CellData.GetCellProxy(pos);
	auto nCell = CELL::NewCell(&m_CellData, pos, rawInput);
	auto oldText = string{ };
	!oldCell ? oldText = ""s : oldText = oldCell->GetRawContent();
	if (rawInput != oldText) { m_UndoStack.emplace_back(oldCell, nCell); m_RedoStack.clear(); }
	return nCell;
}

WINDOWS_TABLE::~WINDOWS_TABLE() { }		// Hook for any on-exit logic

void WINDOWS_TABLE::Redraw() const { }		// Hook for table redraw logic

// Create a new row of cells at bottom edge.
void WINDOWS_TABLE::AddRow() {
	auto cell_ID = CELL_ID{ }.Row(++m_NumRows).Column(0);

	// Create label for row
	auto label = "R"s + to_string(m_NumRows);
	auto window = ConstructChildWindow("Static"s, m_Table, cell_ID);
	auto pos = WINDOW_POSITION{ }.X( m_X0 ).Y( m_Y0 + m_NumRows * m_Height );
	auto size = WINDOW_DIMENSIONS{ }.Height(m_Height).Width(m_Width);
	window.Text(label).Move(pos, size);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.Column() < m_NumColumns) {
		cell_ID.IncrementColumn();
		pos.X(m_X0 + cell_ID.Column() * m_Width).Y(m_Y0 + m_NumRows * m_Height);
		ConstructChildWindow("edit"s, m_Table, cell_ID).Move(pos, size).Procedure(CellWindowProc);
		table->UpdateCell(cell_ID);
	}
}

// Create a new column of cells at right edge.
void WINDOWS_TABLE::AddColumn() {
	auto cell_ID = CELL_ID{ }.Row(0).Column(++m_NumColumns);

	// Create label for column
	auto label = "C"s + to_string(m_NumColumns);
	auto window = ConstructChildWindow("Static"s, m_Table, cell_ID);
	auto pos = WINDOW_POSITION{ }.X( m_X0 + m_NumColumns * m_Width ).Y( m_Y0 );
	auto size = WINDOW_DIMENSIONS{ }.Height(m_Height).Width(m_Width);
	window.Text(label).Move(pos, size);

	// Loop through creating cell windows, filling in display value if it exists
	while (cell_ID.Row() < m_NumRows) {
		cell_ID.IncrementRow();
		pos.X( m_X0 + cell_ID.Column() * m_Width ).Y( m_Y0 + cell_ID.Row() * m_Height );
		ConstructChildWindow("edit"s, m_Table, cell_ID).Move(pos, size).Procedure(CellWindowProc);
		table->UpdateCell(cell_ID);
	}
}

// Remove bottom row of cells.
void WINDOWS_TABLE::RemoveRow() {
	auto id = CELL_ID{ };
	auto h = HWND{ id.Row(m_NumRows) };
	while (id.Column() <= m_NumColumns) { DestroyWindow(h); h = id.IncrementColumn(); }	// Increment through row and destroy cells
	m_NumRows--;
}

// Remove right column of cells.
void WINDOWS_TABLE::RemoveColumn() {
	auto id = CELL_ID{ };
	auto h = HWND{ id.Row(m_NumRows) };
	while (id.Row() <= m_NumRows) { DestroyWindow(h); h = id.IncrementRow(); }	// Increment through column and destroy cells
	m_NumColumns--;
}

// Resize table, creating/destroying cell windows as needed.
void WINDOWS_TABLE::Resize() {
	auto rect = WINDOW{ hParent }.GetClientRect();

	auto col = static_cast<unsigned int>(ceil(static_cast<double>(rect.right) / m_Width) - 1);		// Leave off label column
	auto row = static_cast<unsigned int>(ceil(static_cast<double>(rect.bottom) / m_Height) - 2);	// Leave off label row and Entry box

	// Place hard-cap on row & column number here.
	if (col > MaxColumn_) { col = MaxColumn_ - 1; }
	else if (col < 1) { col = 1; }
	if (row > MaxRow_) { row = MaxRow_ - 1; }
	else if (row < 1) { row = 1; }

	// Add/remove rows and columns until it is exactly filled
	// Each run, at least two while loops will not trigger
	while (m_NumColumns < col) { AddColumn(); }
	while (m_NumColumns > col) { RemoveColumn(); }
	while (m_NumRows < row) { AddRow(); }
	while (m_NumRows > row) { RemoveRow(); }

	auto size = WINDOW_DIMENSIONS{ }.Width(rect.right).Height(rect.bottom);
	WINDOW{ m_Table }.Resize(size);								// Resize underlying canvas
	WINDOW{ m_TextEditBar }.Resize(size.Height(m_Height));		// Resize upper entry box
}

void WINDOWS_TABLE::UpdateCell(const CELL::CELL_POSITION position) const {
	auto cell = m_CellData.GetCellProxy(position);
	auto text = string{ };
	!cell ? text = ""s : text = cell->GetOutput();
	WINDOW{ CELL_ID{ position } }.Text(text);
}

// Manage Focusing a cell in various contexts
void WINDOWS_TABLE::FocusCell(const CELL::CELL_POSITION pos) const {
	auto window = WINDOW{ CELL_ID{ pos } };
	auto cell = m_CellData.GetCellProxy(pos);
	auto text = string{ };
	m_MostRecentCell = pos;
	if (pos == m_PosTargetCell) {						// If returning focus from upper-entry box...
		ReleaseTargetCell();							// Release focus when selecting straight to target to open it up for new target selection.
		CreateNewCell(pos, m_TextEditBar.Text());		// Create new cell from upper entry bar text
		window.Focus();									// Set focus to self to avoid odd errors with changing focus to next cell
	}
	else if (cell) {									// If cell exists
		text = cell->GetRawContent();					// Grab display text of cell
		window.Text(text).Message(EM_SETSEL, 0, -1);	// Show raw content when editing and start by selecting the whole content
		m_TextEditBar.Text(text);						// Display raw content in entry bar
	}
	else { m_TextEditBar.Text(""s); }					// Otherwise, clear entry bar.
}

void WINDOWS_TABLE::UnfocusCell(const CELL::CELL_POSITION pos) const {
	CreateNewCell(pos, WINDOW{ CELL_ID{ pos } }.Text());
}

// Set text to that of target cell to continue editing
// If there is a target, don't reset text
void WINDOWS_TABLE::FocusEntryBox() const {
	if (m_PosTargetCell != CELL::CELL_POSITION{ }) { return; }	
	LockTargetCell(m_MostRecentCell);
	auto text = WINDOW{ CELL_ID{ m_MostRecentCell } }.Text();
	m_TextEditBar.Text(text);
}

// If user clicks another cell while entering text, insert a reference to the selected cell
void WINDOWS_TABLE::UnfocusEntryBox(const CELL::CELL_POSITION pos) const {
	auto id = CELL_ID{ pos };
	if (m_PosTargetCell != CELL::CELL_POSITION{ } && pos != m_PosTargetCell) {
		auto out = wstring{ L"&" };
		out += L"R" + to_wstring(id.Row());
		out += L"C" + to_wstring(id.Column());
		m_TextEditBar.Focus().Message(EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(out.c_str()));
	}
}

void WINDOWS_TABLE::FocusUp1(const CELL::CELL_POSITION pos) const {
	auto id = CELL_ID{ pos };
	if (id.Row() == 1) { return; }
	SetFocus(id.DecrementRow());
}

void WINDOWS_TABLE::FocusDown1(const CELL::CELL_POSITION pos) const {
	auto id = CELL_ID{ pos };
	if (id.Row() >= table->GetNumRows()) { return; }
	SetFocus(id.IncrementRow());
}

void WINDOWS_TABLE::FocusRight1(const CELL::CELL_POSITION pos) const {
	auto id = CELL_ID{ pos };
	if (id.Column() >= table->GetNumColumns()) { return; }
	SetFocus(id.IncrementColumn());
}

void WINDOWS_TABLE::FocusLeft1(const CELL::CELL_POSITION pos) const {
	auto id = CELL_ID{ pos };
	if (id.Column() == 1) { return; }
	SetFocus(id.DecrementColumn());
}

void WINDOWS_TABLE::LockTargetCell(const CELL::CELL_POSITION pos) const 
	{ if (m_PosTargetCell != CELL::CELL_POSITION{ }) { return; } m_PosTargetCell = pos; }

void WINDOWS_TABLE::ReleaseTargetCell() const { m_PosTargetCell = CELL::CELL_POSITION{ }; }

CELL::CELL_POSITION WINDOWS_TABLE::TargetCellGet() const { return m_PosTargetCell; }

// Transition: Cell B -> Cell A
// Move the transition pair {Cell A -> Cell B} onto redo stack
// If Cell A is NULL, then infer its position from Cell B
void WINDOWS_TABLE::Undo() const {
	if (m_UndoStack.empty()) { return; }
	auto cell = m_UndoStack.back().first;
	auto otherCell = m_UndoStack.back().second;
	auto pos = CELL::CELL_POSITION{ };
	if (cell) { pos = cell->GetPosition(); }
	else { pos = otherCell->GetPosition(); }
	CELL::RecreateCell(&m_CellData, cell, pos);			// Null cells need their position
	m_RedoStack.push_back(m_UndoStack.back());
	m_UndoStack.pop_back();
}

// Transition: Cell A -> Cell B
// Move the transition pair {Cell A -> Cell B} onto undo stack
// If Cell B is NULL, then infer its position from the Cell A
void WINDOWS_TABLE::Redo() const {
	if (m_RedoStack.empty()) { return; }
	auto cell = m_RedoStack.back().second;
	auto otherCell = m_RedoStack.back().first;
	auto pos = CELL::CELL_POSITION{ };
	if (cell) { pos = cell->GetPosition(); }
	else { pos = otherCell->GetPosition(); }
	CELL::RecreateCell(&m_CellData, cell, pos);
	m_UndoStack.push_back(m_RedoStack.back());
	m_RedoStack.pop_back();
}
