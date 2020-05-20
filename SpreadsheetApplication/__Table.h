#ifndef TABLE_CLASS_H
#define TABLE_CLASS_H
#include "stdafx.h"
#include "__Cell.h"

class TABLE_BASE;
inline auto table = std::unique_ptr<TABLE_BASE>{ };
constexpr auto MaxRow_{ UINT16_MAX };
constexpr auto MaxColumn_{ UINT16_MAX };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
// Implementations of this class act as an "Adapter" pattern to map platform specific GUI operations to a generic interface.
// Further abstractions may be considered in the future, such as cell windows, graphs, etc.
// I think none of these throw, so I'll add noexcept specification.
class TABLE_BASE {
public:
	virtual void AddRow() noexcept = 0;
	virtual void AddColumn() noexcept = 0;
	virtual void RemoveRow() noexcept = 0;
	virtual void RemoveColumn() noexcept = 0;
	virtual unsigned int GetNumColumns() const noexcept = 0;
	virtual unsigned int GetNumRows() const noexcept = 0;

	virtual void InitializeTable() noexcept = 0;
	virtual void Resize() noexcept = 0;
	virtual void Redraw() const noexcept = 0;

	virtual void FocusCell(CELL::CELL_POSITION) noexcept = 0;
	virtual void UnfocusCell(CELL::CELL_POSITION) noexcept = 0;
	virtual void FocusEntryBox() noexcept = 0;
	virtual void UnfocusEntryBox(CELL::CELL_POSITION) noexcept = 0;

	virtual void FocusUp1(CELL::CELL_POSITION) noexcept = 0;
	virtual void FocusDown1(CELL::CELL_POSITION) noexcept = 0;
	virtual void FocusRight1(CELL::CELL_POSITION) noexcept = 0;
	virtual void FocusLeft1(CELL::CELL_POSITION) noexcept = 0;

	virtual CELL::CELL_PROXY CreateNewCell(CELL::CELL_POSITION, std::string) const noexcept = 0;
	virtual void UpdateCell(CELL::CELL_POSITION) const noexcept = 0;
	virtual void LockTargetCell(CELL::CELL_POSITION) noexcept = 0;
	virtual void ReleaseTargetCell() noexcept = 0;
	virtual CELL::CELL_POSITION TargetCellGet() const noexcept = 0;

	virtual void Undo() const noexcept = 0;
	virtual void Redo() const noexcept = 0;
};

// Windows-specific code is segmented with a preprocessor command
// Code for the appropriate system can be selected by this means
#ifdef _WINDOWS
inline HWND hTable, h_Text_Edit_Bar;
inline WNDPROC EditHandler;
LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);

// Table class specialized for a Windows OS GUI
class WINDOWS_TABLE : public TABLE_BASE {
public:
	class CELL_ID;
	class CELL_WINDOW;
protected:
	unsigned int numColumns{ 0 };
	unsigned int numRows{ 0 };
	int width{ 75 };
	int height{ 25 };
	int x0{ 0 };
	int y0{ 25 };
	HWND hParent;
	CELL::CELL_POSITION origin{ 0, 0 };		// Origin is the "off-the-begining" cell to the upper-left of the upper-left cell
	CELL::CELL_POSITION posTargetCell{ };	// Tracks position of cell currently associated with upper edit box, may be blank
	CELL::CELL_POSITION mostRecentCell{ };	// Tracks position of most recently selected cell for either target selection or new cell creation
	
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> undoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> redoStack{ };
public:
	~WINDOWS_TABLE();						// Hook for any on-exit logic
	void AddRow() noexcept override;
	void AddColumn() noexcept override;
	void RemoveRow() noexcept override;
	void RemoveColumn() noexcept override;
	unsigned int GetNumColumns() const noexcept override { return numColumns; }
	unsigned int GetNumRows() const noexcept override { return numRows; }

	void InitializeTable() noexcept override;
	void Resize() noexcept override;
	void Redraw() const noexcept override;

	void FocusCell(CELL::CELL_POSITION) noexcept override;
	void UnfocusCell(CELL::CELL_POSITION) noexcept override;
	void FocusEntryBox() noexcept override;
	void UnfocusEntryBox(CELL::CELL_POSITION) noexcept override;

	void FocusUp1(CELL::CELL_POSITION) noexcept override;
	void FocusDown1(CELL::CELL_POSITION) noexcept override;
	void FocusRight1(CELL::CELL_POSITION) noexcept override;
	void FocusLeft1(CELL::CELL_POSITION) noexcept override;

	CELL::CELL_PROXY CreateNewCell(CELL::CELL_POSITION, std::string) const noexcept;
	void UpdateCell(CELL::CELL_POSITION) const noexcept override;
	void LockTargetCell(CELL::CELL_POSITION) noexcept override;
	void ReleaseTargetCell() noexcept override;
	CELL::CELL_POSITION TargetCellGet() const noexcept override;

	void Undo() const noexcept override;
	void Redo() const noexcept override;
};

// This is a utility class for converting between window IDs and row/column indicies.
// The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
// The ID will correspond to the row and column number with 16 bits representing each.
// This is scoped within the WINDOWS_TABLE class to indicate to clients that it's intended usage is only within the context of WINDOWS_TABLE usage.
// Define constructors as 'explicit' to avoid any implicit conversions since they take exactly one argument.
// I think none of these throw, so I'll add noexcept specification.
class WINDOWS_TABLE::CELL_ID {
	CELL::CELL_POSITION position;
	unsigned long windowID{ 0 };
	void Win_ID_From_Position() noexcept { windowID = (position.column << 16) + position.row; }
	void Position_From_Win_ID() noexcept {
		position.column = windowID >> 16;
		position.row = (windowID - position.column * (UINT16_MAX + 1));
	}
public:
	CELL_ID() = default;
	explicit CELL_ID(CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	explicit CELL_ID(unsigned long newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }
	explicit CELL_ID(HWND h) : windowID(GetDlgCtrlID(h)) { Position_From_Win_ID(); }

	void SetWindowID(int newID) noexcept { windowID = newID; Position_From_Win_ID(); }
	auto& SetRow(const unsigned int newRowIndex) noexcept { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& SetColumn(const unsigned int newColumnIndex) noexcept { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }
	auto& SetCellPosition(CELL::CELL_POSITION newPosition) noexcept { position = newPosition; Win_ID_From_Position(); return *this; }

	auto GetWindowID() const noexcept { return windowID; }
	auto GetRow() const noexcept { return position.row; }
	auto GetColumn() const noexcept { return position.column; }

	auto& IncrementRow() noexcept { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() noexcept { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() noexcept { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() noexcept { position.column--; Win_ID_From_Position(); return *this; }

	operator CELL::CELL_POSITION() const noexcept { return position; }				// Allow for implicit conversion to CELL_POSITION
	operator HWND() const noexcept { return GetDlgItem(hTable, windowID); }			// Allow for implicit conversion to HWND
	operator HMENU() const noexcept { return reinterpret_cast<HMENU>(windowID); }	// Allow for implicit conversion to HMENU
};

#endif // WIN32

// Alternative table implementaiton for a pure console interface.
// This is provided to show platform portability while minimizing the need for learning a new platform.
#ifdef _CONSOLE
class CONSOLE_TABLE : public TABLE_BASE {
public:
	void Resize() noexcept override;
	void Redraw() const noexcept override;
	void FocusEntryBox() noexcept override;
	void Undo() const noexcept override { }
	void Redo() const noexcept override { }
protected:
	// Unused functions
	void AddRow() noexcept override { }
	void AddColumn() noexcept override { }
	void RemoveRow() noexcept override { }
	void RemoveColumn() noexcept override { }
	unsigned int GetNumColumns() const noexcept override { return 0; }
	unsigned int GetNumRows() const noexcept override { return 0; }
	void InitializeTable() noexcept override { }
	void FocusCell(CELL::CELL_POSITION) noexcept override { }
	void UnfocusCell(CELL::CELL_POSITION) noexcept override { }
	void UnfocusEntryBox(CELL::CELL_POSITION) noexcept override { }
	void FocusUp1(CELL::CELL_POSITION) noexcept override { }
	void FocusDown1(CELL::CELL_POSITION) noexcept override { }
	void FocusRight1(CELL::CELL_POSITION) noexcept override { }
	void FocusLeft1(CELL::CELL_POSITION) noexcept override { }
	void LockTargetCell(CELL::CELL_POSITION) noexcept override { }
	void ReleaseTargetCell() noexcept override { }
	CELL::CELL_PROXY CreateNewCell(CELL::CELL_POSITION, std::string) const noexcept override { return CELL::CELL_PROXY{ }; }
	void UpdateCell(CELL::CELL_POSITION) const noexcept override
	CELL::CELL_POSITION TargetCellGet() const noexcept override { return CELL::CELL_POSITION{ }; }
};
#endif // _CONSOLE


#endif //!TABLE_CLASS_H