#ifndef __TABLE_CLASS
#define __TABLE_CLASS
#include "stdafx.h"
#include "__Cell.h"
#include <vector>

constexpr const auto TABLE_COMMAND_READ_CELL = 1;

class TABLE;

LRESULT CALLBACK TableWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern TABLE table;

//Note here that class TABLE is inherantly designed with the Windows OS in mind.
//This OS dependency could be decoupled by making TABLE into an abstract base class.
//Each OS could then subclass TABLE to implement an appropriate GUI.
class TABLE {
public:
	class CELL_ID;
	class CELL_WINDOW;
private:
	HWND hParent, hTable, hEntryBar;
	HWND h_Text_Edit_Bar;
	int numColumns{ 0 };
	int numRows{ 0 };
	int width{ 75 };
	int height{ 25 };
	int x0{ 0 };
	int y0{ 25 };
	vector<vector<HWND>> cellHandles;

	void AddRow();
	void AddColumn();
	void RemoveRow();
	void RemoveColumn();
public:
	void DrawTableOutline(HWND h);
	void Resize();
};

//This is a utility class for converting between window IDs and row/column indicies.
//The purpose here is to be able to encode the cell position in the window ID, which is stored by the Windows OS.
//This is scoped within the TABLE class to indicate to clients that it's intended usage is only within the context of TABLE usage.
class TABLE::CELL_ID {
	CELL::CELL_POSITION position;
	int windowID{ 0 };
	void Win_ID_From_Position() { windowID = (position.column << 8) | position.row; }
	void Position_From_Win_ID() {
		position.column = windowID >> 8;
		position.row = windowID - position.column;
	}
public:
	CELL_ID(CELL::CELL_POSITION newPosition): position(newPosition) { Win_ID_From_Position(); }
	CELL_ID(int newWindowID): windowID(newWindowID) { Position_From_Win_ID(); }

	void SetWindowID(int newID) { windowID = newID; Position_From_Win_ID(); }
	auto& SetRow(const unsigned int newRowIndex) { position.row = newRowIndex; Win_ID_From_Position(); return *this; }
	auto& SetColumn(const unsigned int newColumnIndex) { position.column = newColumnIndex; Win_ID_From_Position(); return *this; }

	int GetWindowID() { return windowID; }
	int GetRow() { return position.row; }
	int GetColumn() { return position.column; }
};

#endif //!__TABLE_CLASS