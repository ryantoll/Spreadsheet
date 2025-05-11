/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This is an experimental console interface to the spreadsheet application.
// The point here is to show the UI encapsulation & portability of the program.
// To switch over to Windows, change the compiler flag _CONSOLE -> _WINDOWS and change the linker subsystem CONSOLE -> WINDOWS.
// CMake configurations are set up to facilitate easy switching.
// Far fewer table features are needed for this and a few are added to help the console specifically.
*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#undef _WINDOWS

#include <iostream>

#include <memory>

#include <string>

#include "Table.hpp"

using namespace std;

constexpr auto innerCellWidth{ 10 };
constexpr auto numColumns{ 8 };
constexpr auto numRows{ 10 };
constexpr auto addExampleCells{ false };		// Add initial batch of example cells
constexpr auto cellDiagnostics{ false };		// Track cell updates
constexpr auto mainMenu = R"(
Menu:
1. Display Table
2. Edit Cell
3. Clear Cell
4. Undo
5. Redo
6. List All Cells
7. Help
8. Exit
)";
constexpr auto commandHelp = R"(
HELP INFO:
Cell Referece: &R___C___
(Either order; not case-sensitive)

Function Mapping:
(All caps)
(Recursive composition possible)
(References as above)

=SUM(___,___,___)
=AVERAGE(___,___,___)
=PRODUCT(___,___,___)
=RECIPROCAL(___)
=INVERSE(___)
=PI()
)";

int main() {
	table = std::make_unique<CONSOLE_TABLE>();
	cout << R"(
Welcome to the experimental console version of SpreadsheetApplication.
If you are looking for the Windows GUI version, see the instructions for switching between versions.
)" << endl;
	table->InitializeTable();
}

void CONSOLE_TABLE::InitializeTable() noexcept {
	auto input = string{ };
	auto selection = int{ };

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

	cout << '\n' << endl;
	Redraw();
	cout << mainMenu << endl;
	while (cin >> input) {
		try { selection = stoi(input); }
		catch (...) { selection = -1; }

		switch (selection)
		{
		case -1: { cout << "invalid selection\n"; } break;
		case 1: { } break;												// Draw table
		case 2: { CreateNewCell(); } break;								// Create/edit cell, requesting parameters
		case 3: { auto pos = RequestCellPos(); ClearCell(pos); } break;	// Clear cell
		case 4: { Undo(); } break;										// Undo
		case 5: { Redo(); } break;										// Redo
		case 6: { PrintCellList(); } break;								// Print cell list
		case 7: { cout << commandHelp << endl; } break;					// Command list
		case 8: { return; } break;
		default: { cout << "invalid selection\n"; } break;
		}
		Redraw();														// Redraw table after each command
		cout << mainMenu << endl;
	}
}

void CONSOLE_TABLE::Redraw() const noexcept {
	auto pos = CELL::CELL_POSITION{ };
	auto output = string{ };
	for (auto r = 1; r <= numRows; r++) {
		pos.row = r;
		for (auto c = 1; c <= numColumns; c++) {
			pos.column = c;
			auto cell = cellData.GetCellProxy(pos);
			cell ? output = cell->GetOutput() : output = "";
			printf("[%*.*s]", innerCellWidth, innerCellWidth, output.c_str());
		}
		cout << endl;
	}
}

void CONSOLE_TABLE::PrintCellList() const noexcept{
	auto pos = CELL::CELL_POSITION{ };
	auto output = string{ };
	for (auto r = 1; r <= numRows; r++) {
		pos.row = r;
		cout << "Row " << r << " : " << endl;
		for (auto c = 1; c <= numColumns; c++) {
			pos.column = c;
			auto cell = cellData.GetCellProxy(pos);
			if (!cell) { continue; }
			cout << "R" << pos.row << 'C' << pos.column << " -> ";
			printf("%*.*s", innerCellWidth, innerCellWidth, cell->GetOutput().c_str());
			cout << "   " << cell->GetRawContent() << endl;
		}
		cout << endl;
	}
}

void CONSOLE_TABLE::UpdateCell(const CELL::CELL_POSITION pos) const noexcept {
	if (!cellDiagnostics) { return; }
	auto cell = cellData.GetCellProxy(pos);
	if (!cell) { return; }
	cout << "Update Cell: R" << pos.row << 'C' << pos.column << " -> " << cell->GetOutput()
		<< '\t' << cell->GetRawContent() << endl;
}

CELL::CELL_PROXY CONSOLE_TABLE::CreateNewCell() const noexcept {
	auto input = string{ };
	auto pos = RequestCellPos();
	auto current = cellData.GetCellProxy(pos);
	auto display = string{ };	auto raw = string{ };
	if (current) {
		display = current->GetOutput();
		raw = current->GetRawContent();
	}
	cout << "Display: " << display << endl;
	cout << "Raw text: " << raw << endl;
	cout << "New string: ";
	cin >> input;
	auto cell = CreateNewCell(pos, input);
	if (cell) { cout << "Cell successfully created.\n" << endl; }
	else { cout << "Cell creation failed.\n" << endl; }
	return cell;
}

CELL::CELL_PROXY CONSOLE_TABLE::CreateNewCell(const CELL::CELL_POSITION pos, const string& rawInput) const noexcept {
	auto oldCell = cellData.GetCellProxy(pos);
	auto nCell = CELL::NewCell(&cellData, pos, rawInput);
	auto oldText = string{ };
	!oldCell ? oldText = ""s : oldText = oldCell->GetRawContent();
	if (rawInput != oldText) { undoStack.emplace_back(oldCell, nCell); redoStack.clear(); }
	return nCell;
}

void CONSOLE_TABLE::ClearCell(const CELL::CELL_POSITION pos) const noexcept { CreateNewCell(pos, ""s); }

CELL::CELL_POSITION CONSOLE_TABLE::RequestCellPos() const noexcept {
	auto input = string{ };
	auto pos = CELL::CELL_POSITION{ };
	cout << "Select Cell\nR: ";
	cin >> input;
	try { 
	pos.row = stoi(input);
	if (pos.row > numRows || pos.row < 1) { throw exception{"Invalid Input"}; }
	cout << "C: ";
	cin >> input;
	pos.column = stoi(input);
	if (pos.column > numColumns || pos.column < 1) { throw exception{ "Invalid Input" }; }
	}
	catch (...) { cout << "Invalid Input" << endl; pos = RequestCellPos(); }
	return pos;
}

void CONSOLE_TABLE::Undo() const noexcept {
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

void CONSOLE_TABLE::Redo() const noexcept {
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
