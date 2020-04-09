#ifndef __TABLE_CLASS
#define __TABLE_CLASS
#include "stdafx.h"
#include "__Cell.h"
#include <vector>

constexpr const auto TABLE_COMMAND_READ_CELL = 1;

class TABLE_BASE;

//LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
inline unique_ptr<TABLE_BASE> table;

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
class TABLE_BASE {
protected:
	virtual void AddRow() {}
	virtual void AddColumn() {}
	virtual void RemoveRow() {}
	virtual void RemoveColumn() {}
public:
	virtual ~TABLE_BASE() {}
	virtual void DrawTableOutline() {}
	virtual void Resize() {}
	virtual void Redraw() {}
	virtual void UpdateCell(CELL::CELL_POSITION) {}
};

// Table class specialized for a Windows OS GUI
class WINDOWS_TABLE : public TABLE_BASE {
public:
	class CELL_ID;
	class CELL_WINDOW;
	friend LRESULT CALLBACK CellWindowProc(HWND, UINT, WPARAM, LPARAM);
	friend LRESULT CALLBACK EntryBarWindowProc(HWND, UINT, WPARAM, LPARAM);
protected:
	int numColumns{ 0 };
	int numRows{ 0 };
	int width{ 75 };
	int height{ 25 };
	int x0{ 0 };
	int y0{ 25 };
	CELL::CELL_POSITION origin{ 0, 0 };		// Origin is the "off-the-begining" cell to the upper-left of the upper-left cell.

	void AddRow() override;
	void AddColumn() override;
	void RemoveRow() override;
	void RemoveColumn() override;
public:
	void DrawTableOutline() override;
	void Resize() override;
	void Redraw() override;
	void UpdateCell(CELL::CELL_POSITION) override;
};

// This is a utility class for converting between window IDs and row/column indicies.
// The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
// This is scoped within the WINDOWS_TABLE class to indicate to clients that it's intended usage is only within the context of WINDOWS_TABLE usage.
class WINDOWS_TABLE::CELL_ID {
	CELL::CELL_POSITION position;
	int windowID{ 0 };
	void Win_ID_From_Position() { windowID = (position.column << 8) + position.row; }
	void Position_From_Win_ID() {
		position.column = windowID >> 8;
		position.row = (windowID - position.column * 256);
	}
public:
	CELL_ID(CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	CELL_ID(int newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }

	void SetWindowID(int newID) { windowID = newID; Position_From_Win_ID(); }
	auto& SetRow(const unsigned int newRowIndex) { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& SetColumn(const unsigned int newColumnIndex) { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }
	auto& SetCellPosition(CELL::CELL_POSITION& newPosition) { position = newPosition; Win_ID_From_Position(); return *this; }

	int GetWindowID() { return windowID; }
	int GetRow() { return position.row; }
	int GetColumn() { return position.column; }
	auto GetCellPosition() { return position; }

	auto& IncrementRow() { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() { position.column--; Win_ID_From_Position(); return *this; }
};

#endif //!__TABLE_CLASS