#include "framework.hpp"
#include "Utilities.hpp"
#include "Cell.hpp"
#include "Table.hpp"

using namespace std;
using namespace RYANS_UTILITIES;

CELL::CELL_PROXY CELL::NewCell(CELL_DATA* parentContainer, const CELL_POSITION position, const string& contents) noexcept {
	// Check for valid cell position. Disallowing R == 0 && C == 0 not only fits (non-programmer) human intuition,
	// but also prevents accidental errors in failing to specify a location.
	// R == 0 || C == 0 almost certainly indicates a failure to specify one or both arguments.
	if (position.row == 0 || position.column == 0) { return CELL::CELL_PROXY{ nullptr }; }//throw invalid_argument("Neither Row 0, nor Column 0 exist."); }

	// Empty contents argument not only fails to create a new cell, but deletes any cell that may already exist at that position.
	// Notify any observing cells about the change *AFTER* the change has occurred.
	// (Note that control flow immediately goes to any updating cells.)
	auto oldCell = parentContainer->GetCell(position);
	if (contents == "") { if (oldCell) { parentContainer->EraseCell(oldCell->position); parentContainer->NotifyAll(position); } return CELL::CELL_PROXY{ nullptr }; }

	// Avoid re-creating identical CELLs.
	// If it already exists and is built from the same raw string, just return a pointer to the stored CELL.
	if (oldCell && contents == oldCell->rawContent) { table->UpdateCell(position); return CELL::CELL_PROXY{ oldCell }; }

	auto cell = shared_ptr<CELL>();

	auto key = contents[0];
	switch (key){
	case L'\'': { cell = make_shared<TEXT_CELL>(); } break;			// Enforce textual interpretation for format: '__
	case L'&': { cell = make_shared<REFERENCE_CELL>(); } break;		// Takes input in the form of: &R__C__ or &C__R__
	case '=': { cell = make_shared<FUNCTION_CELL>(); } break;		// Partial implementation available
	case L'-': [[fallthrough]];
	case L'.': [[fallthrough]];
	case L'1': [[fallthrough]];
	case L'2': [[fallthrough]];
	case L'3': [[fallthrough]];
	case L'4': [[fallthrough]];
	case L'5': [[fallthrough]];
	case L'6': [[fallthrough]];
	case L'7': [[fallthrough]];
	case L'8': [[fallthrough]];
	case L'9': [[fallthrough]];
	case L'0': { cell = make_shared< NUMERICAL_CELL>(); } break;	// Any cell beginning with a number or decimal is a number.
	default: { cell = make_shared<TEXT_CELL>(); } break;			// By default, all cells are text cells unless otherwise determined.
	}

	cell->position = position;
	cell->rawContent = contents;
	cell->parentContainer = parentContainer;
	parentContainer->AssignCell(cell);			// Add cell to cell map upon creation.

	try { cell->InitializeCell(); }			// Call initialize on cell.
	catch (...) { cell->error = true; }		// Failure of any sort will set the cell into an error state.
	parentContainer->NotifyAll(position);	// Notify any cells that may be observing this position.

	table->UpdateCell(position);						// Notify GUI to update cell value.
	return parentContainer->GetCellProxy(position);		// Return stored cell so that failed numerical cells return the stored fallback text cell rather than the original failed numerical cell.
}

void CELL::RecreateCell(CELL_DATA* parentContainer, const CELL_PROXY& cell, const CELL_POSITION pos) noexcept {
	if (!cell) { parentContainer->EraseCell(pos); }			// Cell stays subscribed.
	else { parentContainer->AssignCell(cell.cell); }
	parentContainer->NotifyAll(pos);
	table->UpdateCell(pos);
}

// Notifies observing CELLs of change in underlying data.
// Each CELL is responsible for checking the new data.
void CELL::CELL_DATA::NotifyAll(const CELL_POSITION subject) const noexcept {
	auto notificationSet = std::set<CELL_POSITION>{ };
	{
		auto lk = lock_guard<mutex>{ data.lkSubMap };		// Lock only to get local copy of notification set
		auto it = data.subscriptionMap.find(subject);
		if (it == data.subscriptionMap.end()) { return; }
		notificationSet = it->second;					// Get local copy of notification set so that lock can be released before updating cells, which will require it's own lock downstream
	}
	for (auto observer: notificationSet) { 
		auto oCell = GetCell(observer);
		if (oCell) { oCell->UpdateCell(); }
	}
}

void CELL::CELL_DATA::AssignCell(const shared_ptr<CELL> cell) noexcept {
	auto lk = lock_guard<mutex>{ data.lkCellMap };
	data.cellMap[cell->position] = cell;
}

void CELL::CELL_DATA::EraseCell(const CELL_POSITION pos) noexcept {
	auto lk = lock_guard<mutex>{ data.lkCellMap };
	data.cellMap.erase(pos);
}

// Subscribe to notification of changes in target CELL.
void CELL::SubscribeToCell(const CELL_POSITION subject) const noexcept { parentContainer->SubscribeToCell(subject, position); }

// Use static overload below
void CELL::UnsubscribeFromCell(const CELL_POSITION subject) const noexcept { parentContainer->UnsubscribeFromCell(subject, position); }

void CELL::CELL_DATA::SubscribeToCell(const CELL_POSITION subject, const CELL_POSITION observer) noexcept {
	auto lk = lock_guard<mutex>{ data.lkSubMap };
	auto& observerSet = data.subscriptionMap[subject];
	observerSet.insert(observer);
}

// Remove observer link (Subject, Observer)
void CELL::CELL_DATA::UnsubscribeFromCell(const CELL_POSITION subject, const CELL_POSITION observer) noexcept {
	auto lk = lock_guard<mutex>{ data.lkSubMap };
	auto itSubject = data.subscriptionMap.find(subject);
	if (itSubject == data.subscriptionMap.end()) { return; }
	(*itSubject).second.erase(observer);
}

std::shared_ptr<CELL> CELL::CELL_DATA::GetCell(const CELL::CELL_POSITION pos) const noexcept {
	auto lk = lock_guard<mutex>{ data.lkCellMap };
	auto it = data.cellMap.find(pos);
	return it != data.cellMap.end() ? it->second : nullptr;
}

CELL::CELL_PROXY CELL::CELL_DATA::GetCellProxy(const CELL::CELL_POSITION pos) noexcept { return CELL_PROXY{ CELL_DATA::GetCell(pos) }; }

void CELL::UpdateCell() noexcept {
	table->UpdateCell(position); 			// Call update cell on GUI base pointer.
	parentContainer->NotifyAll(position);	// Cascade notification
}

void TEXT_CELL::InitializeCell() noexcept {
	CELL::InitializeCell();
	if (displayValue[0] == L'\'') { displayValue.erase(0, 1); }		// Omit preceeding ' if it was added to enforce a text cell
}

// Parese string into Row & Column positions of reference cell
// Parsing allows for either ordering and is not case-sensitive
CELL::CELL_POSITION ReferenceStringToCellPosition(const string& refString) {
	auto row_index = min(refString.find_first_of('R'), refString.find_first_of('r'));
	auto col_index = min(refString.find_first_of('C'), refString.find_first_of('c'));

	auto pos = CELL::CELL_POSITION{ };

	if (col_index > row_index) {
		auto length = col_index - row_index - 1;
		pos.row = stoi(refString.substr(row_index + 1, length));
		pos.column = stoi(refString.substr(col_index + 1));
	}
	else {
		auto length = row_index - col_index - 1;
		pos.column = stoi(refString.substr(col_index + 1, length));
		pos.row = stoi(refString.substr(row_index + 1));
	}
	return pos;
}

// Subscribe to updates on referenced cell once it's position is determined
void REFERENCE_CELL::InitializeCell() noexcept {
	try {
		referencePosition = ReferenceStringToCellPosition(GetRawContent());
		SubscribeToCell(referencePosition);
	}
	catch (...){ error = true; }
}

// Override default error behavior.
// Any cell that seems like a number, but cannot be converted to such defaults to text.
// Create a new cell at the same position with a prepended text-enforcement character.
// Manually check for alpha characters since std::stod() is more forgiving than is appropriate for this situation.
void NUMERICAL_CELL::InitializeCell() noexcept {
	try { 
		for (auto c : GetRawContent()) { if (!isdigit(c) && c != '.' && c != '-') { throw invalid_argument("Error parsing input text. \nText could not be interpreted as a number"); } }
		storedValue = stod(GetRawContent());
	}
	catch (...) { CELL::NewCell(parentContainer, position, "'" + GetRawContent()); }
}

// Parse function text into actual functions.
void FUNCTION_CELL::InitializeCell() noexcept {
	auto inputText = GetRawContent().substr(1);
	auto vArgs = vector<shared_ptr<ARGUMENT>>{ };
	try { 
		vArgs.push_back(ParseFunctionString(inputText));	// Recursively parse input string
		m_Func = make_shared<FUNCTION>(std::move(vArgs));
		storedValue = m_Func->Get();
	}
	catch (...) { m_Func = make_shared<FUNCTION>(); error = true; }
}

// Recalculate function when an underlying reference argument is changed.
void FUNCTION_CELL::UpdateCell() noexcept {
	displayValue = "";
	error = false;		// Reset error flag in case there was a prior error
	try { 
		m_Func->UpdateArgument();
		storedValue = m_Func->Get();
	}
	catch (...) { error = true; }
	CELL::UpdateCell();
}