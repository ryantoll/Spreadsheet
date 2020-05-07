#ifndef __TABLE_CLASS
#define __TABLE_CLASS
#include "stdafx.h"
#include "__Cell.h"

constexpr const auto TABLE_COMMAND_READ_CELL = 1;

class TABLE_BASE;

inline auto table = std::unique_ptr<TABLE_BASE>{ };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
class TABLE_BASE {
protected:
	virtual void AddRow() = 0;
	virtual void AddColumn() = 0;
	virtual void RemoveRow() = 0;
	virtual void RemoveColumn() = 0;
public:
	virtual void DrawTableOutline() = 0;
	virtual void Resize() = 0;
	virtual void Redraw() const = 0;
	virtual void UpdateCell(CELL::CELL_POSITION) const = 0;
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
	~WINDOWS_TABLE();						// Hook for any on-exit logic
	void DrawTableOutline() override;
	void Resize() override;
	void Redraw() const override;
	void UpdateCell(CELL::CELL_POSITION) const override;
};

// This is a utility class for converting between window IDs and row/column indicies.
// The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
// This is scoped within the WINDOWS_TABLE class to indicate to clients that it's intended usage is only within the context of WINDOWS_TABLE usage.
// I think none of these throw, so I'll add noexcept specification.
class WINDOWS_TABLE::CELL_ID {
	CELL::CELL_POSITION position;
	int windowID{ 0 };
	void Win_ID_From_Position() noexcept { windowID = (position.column << 8) + position.row; }
	void Position_From_Win_ID() noexcept {
		position.column = windowID >> 8;
		position.row = (windowID - position.column * 256);
	}
public:
	CELL_ID(CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	CELL_ID(int newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }

	void SetWindowID(int newID) noexcept { windowID = newID; Position_From_Win_ID(); }
	auto& SetRow(const unsigned int newRowIndex) noexcept { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& SetColumn(const unsigned int newColumnIndex) noexcept { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }
	auto& SetCellPosition(CELL::CELL_POSITION newPosition) noexcept { position = newPosition; Win_ID_From_Position(); return *this; }

	int GetWindowID() const noexcept { return windowID; }
	int GetRow() const noexcept { return position.row; }
	int GetColumn() const noexcept { return position.column; }
	auto GetCellPosition() const noexcept { return position; }

	auto& IncrementRow() noexcept { position.row++; Win_ID_From_Position(); return *this; }
	auto& DecrementRow() noexcept { position.row--; Win_ID_From_Position(); return *this; }
	auto& IncrementColumn() noexcept { position.column++; Win_ID_From_Position(); return *this; }
	auto& DecrementColumn() noexcept { position.column--; Win_ID_From_Position(); return *this; }
};

#endif //!__TABLE_CLASS