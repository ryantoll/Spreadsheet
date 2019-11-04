#include "__Cell.h"

shared_ptr<CELL> CELL::CELL_FACTORY::NewCell(CELL_POSITION position, wstring contents) {
	//Check for valid cell position. Disallowing R == 0 && C == 0 not only fits (non-programmer) human intuition,
	//but also prevents accidental errors in failing to specify a location.
	//R == 0 || C == 0 almost certainly indicates a failure to specify one or both arguments.
	if (position.row == 0 || position.column == 0) { throw invalid_argument("Neither Row 0, nor Column 0 exist."); }

	char key = contents[0];
	switch (key){
	case '\'': { cell.reset(new TEXT_CELL()); } break;			//Enforce textual interpretation for format: '__
	case '&': { cell.reset(new REFERENCE_CELL()); } break;		//Takes input in the form of: &R__C__ or &C__R__
	//case '=': { cell.reset(new FUNCTION_CELL()); } break;		//Not yet implemented...
	case '.':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0': { cell.reset(new NUMERICAL_CELL()); } break;		//Any cell beginning with a number or decimal is a number.
	default: { cell.reset(new TEXT_CELL()); } break;			//By default, all cells are text cells unless otherwise determined.
	}

	cell->position = position;
	cell->rawContent = contents;
	
	try { cell->InitializeCell(); }			//Generic error catching for all cell types.
	catch (...) { cell->error = true; }		//Failure of any sort will set the cell into an error state.

	cellMap[position] = cell;

	return cell;
}

void REFERENCE_CELL::InitializeCell() {
	auto row_index = rawReader().find_first_of('R');
	auto col_index = rawReader().find_first_of('C');

	CELL_POSITION pos;
	
	if (col_index > row_index) {
		auto length = col_index - row_index - 1;
		pos.row = stoi(rawReader().substr(row_index + 1, length));
		pos.column = stoi(rawReader().substr(col_index + 1));
	}
	else {
		auto length = row_index - col_index - 1;
		pos.column = stoi(rawReader().substr(col_index + 1, length));
		pos.row = stoi(rawReader().substr(row_index + 1));
	}

	referencePosition;
}

void NUMERICAL_CELL::InitializeCell() {
	//Override default error behavior.
	//Any cell that seems like a number, but cannot be converted to such defaults to text.
	//Create a new cell at the same position with a prepended text-enforcement character.
	try { value = stod(rawReader()); }
	catch (...) { CELL::CELL_FACTORY::NewCell(position, L"'" + rawReader()); }
}