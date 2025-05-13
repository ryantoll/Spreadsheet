#ifndef TABLE_CLASS_HPP
#define TABLE_CLASS_HPP

#include "Cell.hpp"
#include <memory>

#include <string>

class TABLE_BASE;
inline auto table = std::unique_ptr<TABLE_BASE>{ };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
// Implementations of this class act as an "Adapter" pattern to map platform specific GUI operations to a generic interface.
// Further abstractions may be considered in the future, such as cell windows, graphs, etc.
class TABLE_BASE {
public:
	virtual ~TABLE_BASE() { }

	virtual void AddRow() = 0;
	virtual void AddColumn() = 0;
	virtual void RemoveRow() = 0;
	virtual void RemoveColumn() = 0;
	virtual unsigned int GetNumColumns() const = 0;
	virtual unsigned int GetNumRows() const = 0;

	virtual void InitializeTable() = 0;
	virtual void Resize() = 0;
	virtual void Redraw() const = 0;

	virtual void FocusCell(const CELL::CELL_POSITION) const = 0;
	virtual void UnfocusCell(const CELL::CELL_POSITION) const = 0;
	virtual void FocusEntryBox() const = 0;
	virtual void UnfocusEntryBox(const CELL::CELL_POSITION) const = 0;

	virtual void FocusUp1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusDown1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusRight1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusLeft1(const CELL::CELL_POSITION) const = 0;

	virtual CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const = 0;
	virtual void UpdateCell(const CELL::CELL_POSITION) const = 0;
	virtual void LockTargetCell(const CELL::CELL_POSITION) const = 0;
	virtual void ReleaseTargetCell() const = 0;
	virtual CELL::CELL_POSITION TargetCellGet() const = 0;

	virtual void Undo() const = 0;
	virtual void Redo() const = 0;
};

// Windows-specific code is segmented with a preprocessor command
// Code for the appropriate system can be selected by this means
#ifdef _WINDOWS_GUI_APP
#include "WINDOW.hpp"
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
	explicit CELL_ID(const CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	explicit CELL_ID(const unsigned long newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }
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

#endif // _WINDOWS_GUI_APP

// Alternative table implementaiton for a pure console interface.
// This is provided to show platform portability while minimizing the need for learning a new platform.
#ifdef _CONSOLE_APP
class CONSOLE_TABLE : public TABLE_BASE {
public:
	void InitializeTable() override;
	void PrintCellList() const;
	void Redraw() const override;
	void Undo() const override;
	void Redo() const override;
	CELL::CELL_PROXY CreateNewCell() const;
	CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const override;
	void ClearCell(const CELL::CELL_POSITION) const;
	CELL::CELL_POSITION RequestCellPos() const;
protected:
	mutable CELL::CELL_DATA cellData;
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> undoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> redoStack{ };

	// Unused functions
	void Resize() override { }
	void AddRow() override { }
	void AddColumn() override { }
	void RemoveRow() override { }
	void RemoveColumn() override { }
	unsigned int GetNumColumns() const override { return 0; }
	unsigned int GetNumRows() const override { return 0; }
	void FocusCell(const CELL::CELL_POSITION) const override { }
	void UnfocusCell(const CELL::CELL_POSITION) const override { }
	void FocusEntryBox() const override { }
	void UnfocusEntryBox(const CELL::CELL_POSITION) const override { }
	void FocusUp1(const CELL::CELL_POSITION) const override { }
	void FocusDown1(const CELL::CELL_POSITION) const override { }
	void FocusRight1(const CELL::CELL_POSITION) const override { }
	void FocusLeft1(const CELL::CELL_POSITION) const override { }
	void LockTargetCell(const CELL::CELL_POSITION) const override { }
	void ReleaseTargetCell() const override { }
	void UpdateCell(const CELL::CELL_POSITION) const override;
	CELL::CELL_POSITION TargetCellGet() const override { return CELL::CELL_POSITION{ }; }
};
#endif // _CONSOLE_APP


#endif //!TABLE_CLASS_HPP