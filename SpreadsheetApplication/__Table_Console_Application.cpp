#include "stdafx.h"

//#define _CONSOLE

#ifdef _CONSOLE
#include "Utilities.h"
#include "__Table.h"

using std::cin;
using std::cout;
using std::endl;
using std::string;

#include <iostream>

constexpr auto mainMenu = R"(
Menu:
1. Display Table\n
2. Edit Cell\n
3. List All Cells\n
4. Exit
)";

int main() {
	table = std::make_unique<CONSOLE_TABLE>();
	auto input = string{ };
	auto selection = int{ };

	cout << R"(
Welcome to the experimental console version of SpreadsheetApplication.
If you are looking for the Windows GUI version, see the instructions for switching between versions.
)" << endl;

	cout << mainMenu;
	while (cin >> input) {
		selection = std::stoi(input);
		switch (selection)
		{
		case 1: { table->Redraw(); } break;
		case 2: { table->TargetCellGet(); } break;
		case 3: { table->Resize(); } break;
		case 4: { return 0; } break;
		default: { cout << "invalid selection\n"; }
			break;
		}
	}
}

#endif // _CONSOLE