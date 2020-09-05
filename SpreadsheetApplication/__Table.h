#ifndef TABLE_CLASS_H
#define TABLE_CLASS_H
#include "stdafx.h"
#include "__Cell.h"
#include "__WINDOW.h"

class TABLE_BASE;
inline auto table = std::unique_ptr<TABLE_BASE>{ };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
// Implementations of this class act as an "Adapter" pattern to map platform specific GUI operations to a generic interface.
// Further abstractions may be considered in the future, such as cell windows, graphs, etc.
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

	virtual void FocusCell(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void UnfocusCell(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void FocusEntryBox() const noexcept = 0;
	virtual void UnfocusEntryBox(const CELL::CELL_POSITION) const noexcept = 0;

	virtual void FocusUp1(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void FocusDown1(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void FocusRight1(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void FocusLeft1(const CELL::CELL_POSITION) const noexcept = 0;

	virtual CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const noexcept = 0;
	virtual void UpdateCell(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void LockTargetCell(const CELL::CELL_POSITION) const noexcept = 0;
	virtual void ReleaseTargetCell() const noexcept = 0;
	virtual CELL::CELL_POSITION TargetCellGet() const noexcept = 0;

	virtual void Undo() const noexcept = 0;
	virtual void Redo() const noexcept = 0;
};

// Windows-specific code is segmented with a preprocessor command
// Code for the appropriate system can be selected by this means
#ifdef _WINDOWS
inline WINDOWS_GUI::WINDOW m_Table, m_TextEditBar;
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
	HWND hParent;
	mutable CELL::CELL_DATA m_CellData;
	mutable CELL::CELL_POSITION m_Origin{ 0, 0 };		// Origin is the "off-the-begining" cell to the upper-left of the upper-left cell
	mutable CELL::CELL_POSITION m_PosTargetCell{ };	// Tracks position of cell currently associated with upper edit box, may be blank
	mutable CELL::CELL_POSITION m_MostRecentCell{ };	// Tracks position of most recently selected cell for either target selection or new cell creation
	
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> m_UndoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> m_RedoStack{ };
public:
	~WINDOWS_TABLE();						// Hook for any on-exit logic
	void AddRow() noexcept override;
	void AddColumn() noexcept override;
	void RemoveRow() noexcept override;
	void RemoveColumn() noexcept override;
	unsigned int GetNumColumns() const noexcept override { return m_NumColumns; }
	unsigned int GetNumRows() const noexcept override { return m_NumRows; }

	void InitializeTable() noexcept override;
	void Resize() noexcept override;
	void Redraw() const noexcept override;

	void FocusCell(const CELL::CELL_POSITION) const noexcept override;
	void UnfocusCell(const CELL::CELL_POSITION) const noexcept override;
	void FocusEntryBox() const noexcept override;
	void UnfocusEntryBox(const CELL::CELL_POSITION) const noexcept override;

	void FocusUp1(const CELL::CELL_POSITION) const noexcept override;
	void FocusDown1(const CELL::CELL_POSITION) const noexcept override;
	void FocusRight1(const CELL::CELL_POSITION) const noexcept override;
	void FocusLeft1(const CELL::CELL_POSITION) const noexcept override;

	CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const noexcept;
	void UpdateCell(const CELL::CELL_POSITION) const noexcept override;
	void LockTargetCell(const CELL::CELL_POSITION) const noexcept override;
	void ReleaseTargetCell() const noexcept override;
	CELL::CELL_POSITION TargetCellGet() const noexcept override;

	void Undo() const noexcept override;
	void Redo() const noexcept override;
};

// This is a utility class for converting between window IDs and row/column indicies.
// The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
// The ID will correspond to the row and column number with 16 bits representing each.
// This is scoped within the WINDOWS_TABLE class to indicate to clients that it's intended usage is only within the context of WINDOWS_TABLE usage.
// Define constructors as 'explicit' to avoid any implicit conversions since they take exactly one argument.
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
	explicit CELL_ID(const CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	explicit CELL_ID(const unsigned long newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }
	explicit CELL_ID(const HWND h) : windowID(GetDlgCtrlID(h)) { Position_From_Win_ID(); }

	void WindowID(const int newID) noexcept { windowID = newID; Position_From_Win_ID(); }
	auto& Row(const unsigned int newRowIndex) noexcept { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& Column(const unsigned int newColumnIndex) noexcept { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }
	auto& SetCellPosition(const CELL::CELL_POSITION newPosition) noexcept { position = newPosition; Win_ID_From_Position(); return *this; }

	auto WindowID() const noexcept { return windowID; }
	auto Row() const noexcept { return position.row; }
	auto Column() const noexcept { return position.column; }

	auto& IncrementRow() noexcept { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() noexcept { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() noexcept { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() noexcept { position.column--; Win_ID_From_Position(); return *this; }

	operator CELL::CELL_POSITION() const noexcept { return position; }				// Allow for implicit conversion to CELL_POSITION
	operator HWND() const noexcept { return GetDlgItem(m_Table, windowID); }		// Allow for implicit conversion to HWND
	operator HMENU() const noexcept { return reinterpret_cast<HMENU>(windowID); }	// Allow for implicit conversion to HMENU
};

#endif // _WINDOWS

// Alternative table implementaiton for a pure console interface.
// This is provided to show platform portability while minimizing the need for learning a new platform.
#ifdef _CONSOLE
class CONSOLE_TABLE : public TABLE_BASE {
public:
	void InitializeTable() noexcept override;
	void PrintCellList() const noexcept;
	void Redraw() const noexcept override;
	void Undo() const noexcept override;
	void Redo() const noexcept override;
	CELL::CELL_PROXY CreateNewCell() const noexcept;
	CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const noexcept override;
	void ClearCell(const CELL::CELL_POSITION) const noexcept;
	CELL::CELL_POSITION RequestCellPos() const noexcept;
protected:
	mutable CELL::CELL_DATA cellData;
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> undoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> redoStack{ };

	// Unused functions
	void Resize() noexcept override { }
	void AddRow() noexcept override { }
	void AddColumn() noexcept override { }
	void RemoveRow() noexcept override { }
	void RemoveColumn() noexcept override { }
	unsigned int GetNumColumns() const noexcept override { return 0; }
	unsigned int GetNumRows() const noexcept override { return 0; }
	void FocusCell(const CELL::CELL_POSITION) const noexcept override { }
	void UnfocusCell(const CELL::CELL_POSITION) const noexcept override { }
	void FocusEntryBox() const noexcept override { }
	void UnfocusEntryBox(const CELL::CELL_POSITION) const noexcept override { }
	void FocusUp1(const CELL::CELL_POSITION) const noexcept override { }
	void FocusDown1(const CELL::CELL_POSITION) const noexcept override { }
	void FocusRight1(const CELL::CELL_POSITION) const noexcept override { }
	void FocusLeft1(const CELL::CELL_POSITION) const noexcept override { }
	void LockTargetCell(const CELL::CELL_POSITION) const noexcept override { }
	void ReleaseTargetCell() const noexcept override { }
	void UpdateCell(const CELL::CELL_POSITION) const noexcept override;
	CELL::CELL_POSITION TargetCellGet() const noexcept override { return CELL::CELL_POSITION{ }; }
};
#endif // _CONSOLE


#endif //!TABLE_CLASS_H