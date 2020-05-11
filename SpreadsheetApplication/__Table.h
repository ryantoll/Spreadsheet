#ifndef __TABLE_CLASS
#define __TABLE_CLASS
#include "stdafx.h"
#include "__Cell.h"

constexpr const auto TABLE_COMMAND_READ_CELL = 1;

class TABLE_BASE;
inline auto table = std::unique_ptr<TABLE_BASE>{ };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
// Implementations of this class act as a "Bridge" pattern to map platform specific GUI operations to a generic interface.
// I think none of these throw, so I'll add noexcept specification.
class TABLE_BASE {
public:
	virtual void AddRow() noexcept = 0;
	virtual void AddColumn() noexcept = 0;
	virtual void RemoveRow() noexcept = 0;
	virtual void RemoveColumn() noexcept = 0;
	virtual unsigned int GetNumColumns() const noexcept = 0;
	virtual unsigned int GetNumRows() const noexcept = 0;

	virtual void DrawTableOutline() noexcept = 0;
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

	virtual std::shared_ptr<CELL> CreateNewCell(CELL::CELL_POSITION, std::string) const noexcept = 0;
	virtual void UpdateCell(CELL::CELL_POSITION) const noexcept = 0;
	virtual void LockTargetCell(CELL::CELL_POSITION) noexcept = 0;
	virtual void ReleaseTargetCell() noexcept = 0;
	virtual CELL::CELL_POSITION TargetCellGet() const noexcept = 0;
};

// Windows-specific code is segmented with a preprocessor command
// Code for the appropriate system can be selected by this means
#ifdef WIN32
inline WNDPROC EditHandler;
inline HWND hParent, hTable, h_Text_Edit_Bar;
LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);

// Table class specialized for a Windows OS GUI
class WINDOWS_TABLE : public TABLE_BASE {
public:
	class CELL_ID;
	class CELL_WINDOW;
protected:
	int numColumns{ 0 };
	int numRows{ 0 };
	int width{ 75 };
	int height{ 25 };
	int x0{ 0 };
	int y0{ 25 };
	CELL::CELL_POSITION origin{ 0, 0 };		// Origin is the "off-the-begining" cell to the upper-left of the upper-left cell
	CELL::CELL_POSITION posTargetCell{ };	// Tracks position of cell currently associated with upper edit box, may be blank
	CELL::CELL_POSITION mostRecentCell{ };	// Tracks position of most recently selected cell for either target selection or new cell creation
public:
	~WINDOWS_TABLE();						// Hook for any on-exit logic
	void AddRow() noexcept override;
	void AddColumn() noexcept override;
	void RemoveRow() noexcept override;
	void RemoveColumn() noexcept override;
	unsigned int GetNumColumns() const noexcept override { return numColumns; }
	unsigned int GetNumRows() const noexcept override { return numRows; }

	void DrawTableOutline() noexcept override;
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

	std::shared_ptr<CELL> CreateNewCell(CELL::CELL_POSITION, std::string) const noexcept;
	void UpdateCell(CELL::CELL_POSITION) const noexcept override;
	void LockTargetCell(CELL::CELL_POSITION) noexcept override;
	void ReleaseTargetCell() noexcept override;
	CELL::CELL_POSITION TargetCellGet() const noexcept override;
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
		position.row = (windowID - position.column * 65536);
	}
public:
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
	auto GetCellPosition() const noexcept { return position; }
	HWND GetWindowHandle() const noexcept;	// Defined in .cpp file to have access to the parent table handle

	auto& IncrementRow() noexcept { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() noexcept { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() noexcept { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() noexcept { position.column--; Win_ID_From_Position(); return *this; }
};

#endif // WIN32

#endif //!__TABLE_CLASS