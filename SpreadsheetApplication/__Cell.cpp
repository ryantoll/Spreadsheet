#include "stdafx.h"
#include "__Cell.h"
#include "__Table.h"
#include <stdexcept>

//multimap<CELL::CELL_POSITION, CELL::CELL_POSITION> CELL::subscriptionMap = { }; 	//<Subject, Observers>
multimap<CELL::CELL_POSITION, CELL::CELL_POSITION> subscriptionMap{ }; 				//<Subject, Observers>

//map<wstring, shared_ptr<FUNCTION_CELL::FUNCTION>> functionNameMap{ {wstring(L"SUM"), shared_ptr<FUNCTION_CELL::SUM>()}, {wstring(L"AVERAGE"), shared_ptr<FUNCTION_CELL::AVERAGE>()} };

shared_ptr<CELL> CELL::CELL_FACTORY::NewCell(CELL_POSITION position, wstring contents) {
	// Check for valid cell position. Disallowing R == 0 && C == 0 not only fits (non-programmer) human intuition,
	// but also prevents accidental errors in failing to specify a location.
	// R == 0 || C == 0 almost certainly indicates a failure to specify one or both arguments.
	if (position.row == 0 || position.column == 0) { throw invalid_argument("Neither Row 0, nor Column 0 exist."); }

	// Empty contents argument not only fails to create a new cell, but deletes any cell that may already exist at that position.
	// Notify any observing cells about the change *AFTER* the change has occurred.
	// (Note that control flow immediately goes to any updating cells.)
	if (contents == L"") { cellMap.erase(position); NotifyAll(position); return nullptr; }

	shared_ptr<CELL> cell;

	wchar_t key = contents[0];
	switch (key){
	case L'\'': { cell.reset(new TEXT_CELL()); } break;			// Enforce textual interpretation for format: '__
	case L'&': { cell.reset(new REFERENCE_CELL()); } break;		// Takes input in the form of: &R__C__ or &C__R__
	case '=': { cell.reset(new FUNCTION_CELL()); } break;		// Not yet implemented...
	case L'.':
	case L'1':
	case L'2':
	case L'3':
	case L'4':
	case L'5':
	case L'6':
	case L'7':
	case L'8':
	case L'9':
	case L'0': { cell.reset(new NUMERICAL_CELL()); } break;		// Any cell beginning with a number or decimal is a number.
	default: { cell.reset(new TEXT_CELL()); } break;			// By default, all cells are text cells unless otherwise determined.
	}

	cell->position = position;
	cell->rawContent = contents;
	
	try { cell->InitializeCell(); }			// Generic error catching for all cell types.
	catch (...) { cell->error = true; }		// Failure of any sort will set the cell into an error state.

	cellMap[position] = cell;				// Add cell to cell map upon creation.
	NotifyAll(position);					// Notify any cells that may be observing this position.

	// Factory is entirely done with cell.
	// Use std::move() to ensure no dangling reference allows for future erroneous alterations.
	return std::move(cell);
}

void CELL::CELL_FACTORY::NotifyAll(CELL_POSITION subject) {
	auto beg = subscriptionMap.lower_bound(subject);
	auto end = subscriptionMap.upper_bound(subject);

	while (beg != end) {
		try { cellMap[beg->second]->UpdateCell(); }
		catch (...) {}	// May throw when closing program. Irrelevant except that we don't want the program to crash on exit.
		beg++;
	}
}

bool CELL::MoveCell(CELL_POSITION newPosition) {
	if (cellMap.find(newPosition) != cellMap.end()) { return false; }
	cellMap[newPosition] = cellMap[position];
	cellMap.erase(position);
	position = newPosition;
	return true;
}

void CELL::SubscribeToCell(CELL_POSITION subject) { subscriptionMap.insert({ subject, position }); }

void CELL::UnsubscribeFromCell(CELL_POSITION subject) {
	if (subscriptionMap.size() == 0) { return; }
	auto beg = subscriptionMap.lower_bound(subject);
	auto end = subscriptionMap.upper_bound(subject);

	// Iterate through all elements with subject as key.
	// Only erase the one where this cell is the observer.
	while (beg != end) {		
		if (beg->second == position) { subscriptionMap.erase(beg); break; }
		else { beg++; }
	}
}

void CELL::UpdateCell() { table->UpdateCell(position); }		// Call update cell on GUI base pointer.

// Parese string into Row & Column positions of reference cell
// Parsing allows for either ordering and is not case-sensitive
// Subscribe to updates on referenced cell once it's position is determined
void REFERENCE_CELL::InitializeCell() {
	auto row_index = min(rawReader().find_first_of('R'), rawReader().find_first_of('r'));
	auto col_index = min(rawReader().find_first_of('C'), rawReader().find_first_of('c'));

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

	referencePosition = pos;
	SubscribeToCell(referencePosition);
}

void NUMERICAL_CELL::InitializeCell() {
	// Override default error behavior.
	// Any cell that seems like a number, but cannot be converted to such defaults to text.
	// Create a new cell at the same position with a prepended text-enforcement character.
	try { storedValue = stod(rawReader()); }
	catch (...) { CELL::CELL_FACTORY::NewCell(position, L"'" + rawReader()); }
}

shared_ptr<FUNCTION_CELL::FUNCTION> MatchNameToFunction(wstring& inputText) {
	//return functionNameMap.find(inputText)->second;
	//return make_shared<FUNCTION_CELL::SUM>();
	
	if (inputText == L"SUM") { return make_shared<FUNCTION_CELL::SUM>(); }
	else if (inputText == L"AVERAGE") { return make_shared<FUNCTION_CELL::AVERAGE>(); }

}

// As I write this, I realize how complicated this parsing can become.
// This approach may get overly cumbersome once I account for operators (+,-,*,/) as well as ordering parentheses.
// I have seen other parsing solutions on a superficial level and they break down the text into "token" objects.
// I may need to rework this in a more object-oriented solution to make it easier to conceptualize the various complexities.
shared_ptr<FUNCTION_CELL::ARGUMENT> FUNCTION_CELL::ParseFunctionString(wstring& inputText) {
	// Add "clear white space" operation
	// Add "clear brackets" operation

	if (isalpha(inputText[0])) { /*Convert function name*/ 
		auto n = 0;
		while (isalpha(inputText[n])) { ++n; }
		auto funcName = inputText.substr(0, n);
		inputText.erase(0, n);

		// Create approprate function cell from function name
		// Call new parsing operation to fill out function arguments
		//auto func = FUNCTION_CELL::SUM();	// Placeholder for return value

		auto func = MatchNameToFunction(funcName); func->Arguments.clear();
		if (!ClearEnclosingChars(L'(', L')', inputText)) { throw invalid_argument("Error parsing input text. \nParentheses mismatch."); }

		// Segment text within parentheses into segments deliniated by commas
		// This will fill the vector of ARGUMENTS used for function input parameters
		// Needs more work to sort out nested function arguments
		// Always grabs next comma even if it's from a nested function
		vector<wstring> argSegments;
		n = inputText.find_first_of(L',');
		while (n != wstring::npos) {
			argSegments.push_back(inputText.substr(0, n));
			inputText.erase(0, n + 1);
			n = inputText.find_first_of(L',');
		}
		argSegments.push_back(inputText.substr(0, n));

		for (auto arg : argSegments) { func->Arguments.push_back(ParseFunctionString(arg)); }	// For each segment, build it into an argument recursively
		func->Function();																		// Associate future with result of function
		return func;
	}
	else if (inputText[0] == '&') { /*Convert reference*/ 
		// Literally copied from reference cell initiation. Consider unifying into one funciton for ease of maintainance.
		auto row_index = min(inputText.find_first_of('R'), inputText.find_first_of('r'));
		auto col_index = min(inputText.find_first_of('C'), inputText.find_first_of('c'));

		CELL::CELL_POSITION pos;

		if (col_index > row_index) {
			auto length = col_index - row_index - 1;
			pos.row = stoi(inputText.substr(row_index + 1, length));
			pos.column = stoi(inputText.substr(col_index + 1));
		}
		else {
			auto length = row_index - col_index - 1;
			pos.column = stoi(inputText.substr(col_index + 1, length));
			pos.row = stoi(inputText.substr(row_index + 1));
		}

		auto referencePosition = pos;
		SubscribeToCell(referencePosition);
		return make_shared<FUNCTION_CELL::REFERENCE_ARGUMENT>(pos);
	}
	else if (isdigit(inputText[0])) { /*Convert to value*/ 
		auto n = 0;
		while (isdigit(inputText[n])) { ++n; }
		auto num = inputText.substr(0, n);
		inputText.erase(0, n);
		return make_shared<FUNCTION_CELL::VALUE_ARGUMENT>(stod(num));	// wstring -> double -> Value Argument
	}
	else { throw invalid_argument("Error parsing input text."); }	/*Set error flag*/
}

// Parse function text into actual functions.
void FUNCTION_CELL::InitializeCell() {
	auto inputText = rawReader().substr(1);
	func.Arguments.push_back(ParseFunctionString(inputText));		// Recursively parse input string
	func.Function();												// Associate future with result of function
	storedValue = func.val.get();
}

void FUNCTION_CELL::FUNCTION::Function() {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value((*Arguments.begin())->val.get());		// By default, assume a single argument and simply grab its value
}

FUNCTION_CELL::VALUE_ARGUMENT::VALUE_ARGUMENT(double arg) {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(arg);
}

// May throw
// Look up value upon creation
FUNCTION_CELL::REFERENCE_ARGUMENT::REFERENCE_ARGUMENT(CELL_POSITION pos) : referencePosition(pos) {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(stod(cellMap.at(pos)->DisplayOutput()));	// Stored value may need to be tracked separately from display value eventually
}

void FUNCTION_CELL::SUM::Function() {
	//val = async(std::launch::async | std::launch::deferred, [&input] { return std::accumulate(input.begin(), input.end(), 0.0); });
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred, [&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->val.get(); }  return sum; });
}

void FUNCTION_CELL::AVERAGE::Function() {
	//val = async(std::launch::async | std::launch::deferred, [&input] { return std::accumulate(input.begin(), input.end(), 0.0) / input.size(); });
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred, [&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->val.get(); }  return sum / input.size(); });
}