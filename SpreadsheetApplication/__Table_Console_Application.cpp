#include "stdafx.h"

//#define _CONSOLE

#ifdef _CONSOLE
#include "Utilities.h"
#include "__Table.h"

using namespace std;

constexpr auto innerCellWidth{ 10 };
constexpr auto maxColumns{ 8 };
constexpr auto maxRows{ 10 };
constexpr auto cellDiagnostics{ true };		// Track cell updates
constexpr auto mainMenu = R"(
Menu:
1. Display Table
2. Edit Cell
3. List All Cells
4. Help
5. Exit
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
	auto input = string{ };
	auto selection = int{ };

	cout << R"(
Welcome to the experimental console version of SpreadsheetApplication.
If you are looking for the Windows GUI version, see the instructions for switching between versions.
)" << endl;


	// Insert cells upon creation (Optional)
	CELL::cell_factory.NewCell({ 1, 1 }, "2");
	CELL::cell_factory.NewCell({ 1, 2 }, "4");
	CELL::cell_factory.NewCell({ 1, 3 }, "9");
	CELL::cell_factory.NewCell({ 1, 4 }, "SUM ->");
	CELL::cell_factory.NewCell({ 1, 5 }, "=SUM( &R1C1, &R1C2, &R1C3 )");

	CELL::cell_factory.NewCell({ 2, 1 }, "=RECIPROCAL( &R1C1 )");
	CELL::cell_factory.NewCell({ 2, 2 }, "=INVERSE( &R1C2 )");
	CELL::cell_factory.NewCell({ 2, 3 }, "=&R1C3");
	CELL::cell_factory.NewCell({ 2, 4 }, "SUM ->");
	CELL::cell_factory.NewCell({ 2, 5 }, "=SUM( &R2C1, &R2C2, &R2C3 )");

	CELL::cell_factory.NewCell({ 3, 1 }, "&R2C1");
	CELL::cell_factory.NewCell({ 3, 2 }, "&R2C2");
	CELL::cell_factory.NewCell({ 3, 3 }, "&R2C3");
	CELL::cell_factory.NewCell({ 3, 4 }, "AVERAGE ->");
	CELL::cell_factory.NewCell({ 3, 5 }, "=AVERAGE( &R3C1, &R3C2, &R3C3 )");

	cout << '\n' << endl;
	table->Redraw();
	cout << mainMenu << endl;
	while (cin >> input) {
		try { selection = stoi(input); }
		catch (...) { selection = -1; }
		
		switch (selection)
		{
		case -1: { cout << "invalid selection\n"; }
		case 1: { table->Redraw(); } break;
		case 2: { table->FocusEntryBox(); } break;
		case 3: { table->Resize(); } break;
		case 4: { cout << commandHelp << endl; } break;
		case 5: { return 0; } break;
		default: { cout << "invalid selection\n"; }
			break;
		}
		cout << mainMenu << endl;
	}
}

void CONSOLE_TABLE::Redraw() const noexcept {
	auto pos = CELL::CELL_POSITION{ };
	auto output = string{ };
	for (auto r = 1; r <= maxRows; r++) {
		pos.row = r;
		for (auto c = 1; c <= maxColumns; c++) {
			pos.column = c;
			auto cell = CELL::GetCellProxy(pos);
			cell ? output = cell->DisplayOutput() : output = "";
			printf("[%*.*s]", innerCellWidth, innerCellWidth, output.c_str());
		}
		cout << endl;
	}
}

void CONSOLE_TABLE::Resize() noexcept {
	auto pos = CELL::CELL_POSITION{ };
	auto output = string{ };
	for (auto r = 1; r <= maxRows; r++) {
		pos.row = r;
		cout << "Row " << r << " : " << endl;
		for (auto c = 1; c <= maxColumns; c++) {
			pos.column = c;
			auto cell = CELL::GetCellProxy(pos);
			if (!cell) { continue; }
			cout << "R" << pos.row << 'C' << pos.column << " -> ";
			printf("%*.*s", innerCellWidth, innerCellWidth, cell->DisplayOutput().c_str());
			cout << "   " << cell->DisplayRawContent() << endl;
		}
		cout << endl;
	}
}

void CONSOLE_TABLE::FocusEntryBox() noexcept {
	auto input = string{ };
	auto pos = CELL::CELL_POSITION{ };
	cout << "Select Cell\nR: ";
	cin >> input;
	try { pos.row = stoi(input); }
	catch (...) { cout << "Invalid Input\nCell creation failed.\n"; return; }
	cout << "C: ";
	cin >> input;
	try { pos.column = stoi(input); }
	catch (...) { cout << "Invalid Input\nCell creation failed.\n"; return; }
	
	auto current = CELL::GetCellProxy(pos);
	auto display = string{ };	auto raw = string{ };
	if (current) {
		display = current->DisplayOutput();
		raw = current->DisplayRawContent();
	}
	cout << "Display: " << display << endl;
	cout << "Raw text: " << raw << endl;
	cout << "New string: ";
	cin >> input;
	auto cell = CELL::cell_factory.NewCell(pos, input);
	if (cell) { cout << "Cell successfully created.\n" << endl; }
	else { cout << "Cell creation failed.\n" << endl; }
	Redraw();
}

void CONSOLE_TABLE::UpdateCell(CELL::CELL_POSITION pos) const noexcept {
	if (!cellDiagnostics) { return; }
	auto cell = CELL::GetCellProxy(pos);
	if (!cell) { return; }
	cout << "Update Cell: R" << pos.row << 'C' << pos.column << " -> " << cell->DisplayOutput() 
		<< '\t' << cell->DisplayRawContent() << endl;
}

#endif // _CONSOLE