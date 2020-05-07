#include "stdafx.h"
#include "Utilities.h"
#include "__Cell.h"
#include "__Table.h"
#include <stdexcept>

using std::string;
using std::vector;
using std::shared_ptr;
using std::promise;
using std::invalid_argument;
using std::make_shared;
using namespace RYANS_UTILITIES;

auto subscriptionMap = std::multimap<CELL::CELL_POSITION, CELL::CELL_POSITION>{ }; 				//<Subject, Observers>

// Part of an alternative function mapping scheme
//map<wstring, shared_ptr<FUNCTION_CELL::FUNCTION>> functionNameMap{ {wstring(L"SUM"), shared_ptr<FUNCTION_CELL::SUM>()}, {wstring(L"AVERAGE"), shared_ptr<FUNCTION_CELL::AVERAGE>()} };

shared_ptr<CELL> CELL::CELL_FACTORY::NewCell(CELL_POSITION position, const string& contents) {
	// Check for valid cell position. Disallowing R == 0 && C == 0 not only fits (non-programmer) human intuition,
	// but also prevents accidental errors in failing to specify a location.
	// R == 0 || C == 0 almost certainly indicates a failure to specify one or both arguments.
	if (position.row == 0 || position.column == 0) { throw invalid_argument("Neither Row 0, nor Column 0 exist."); }

	// Empty contents argument not only fails to create a new cell, but deletes any cell that may already exist at that position.
	// Notify any observing cells about the change *AFTER* the change has occurred.
	// (Note that control flow immediately goes to any updating cells.)
	if (contents == "") { cellMap.erase(position); NotifyAll(position); return nullptr; }

	auto cell = shared_ptr<CELL>();

	auto key = contents[0];
	switch (key){
	case L'\'': { cell = make_shared<TEXT_CELL>(); } break;			// Enforce textual interpretation for format: '__
	case L'&': { cell = make_shared<REFERENCE_CELL>(); } break;		// Takes input in the form of: &R__C__ or &C__R__
	case '=': { cell = make_shared<FUNCTION_CELL>(); } break;		// Partial implementation available
	case L'-':
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
	case L'0': { cell = make_shared< NUMERICAL_CELL>(); } break;	// Any cell beginning with a number or decimal is a number.
	default: { cell = make_shared<TEXT_CELL>(); } break;			// By default, all cells are text cells unless otherwise determined.
	}

	cell->position = position;
	cell->rawContent = contents;

	cellMap[position] = cell;				// Add cell to cell map upon creation.
	try { cell->InitializeCell(); }			// Call initialize on cell.
	catch (...) { cell->error = true; }		// Failure of any sort will set the cell into an error state.
	NotifyAll(position);					// Notify any cells that may be observing this position.

	return cellMap[position];				// Return stored cell so that failed numerical cells return the stored fallback text cell rather than the original failed numerical cell.
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

void CELL::SubscribeToCell(CELL_POSITION subject) const { subscriptionMap.insert({ subject, position }); }

void CELL::UnsubscribeFromCell(CELL_POSITION subject) const {
	if (subscriptionMap.size() == 0) { return; }
	try {
		auto beg = subscriptionMap.lower_bound(subject);
		auto end = subscriptionMap.upper_bound(subject);

		// Iterate through all elements with subject as key.
		// Only erase the one where this cell is the observer.
		while (beg != end) {
			if (beg->second == position) { subscriptionMap.erase(beg); break; }
			else { beg++; }
		}
	}
	// I don't know why, but sometimes upon application exit, the container invariants are broken
	// The container size != 0, but every entry comes up as "Unable to read memory" in debugger
	// An exception is thrown trying to access this invalid memory
	// Upon program exit, a failed unsubscription is meaningless, though quite unexpected
	// The catch block should handle the error so that the program exits gracefully and any other exit proceedures run as normal
	// Still, it seems to be throwing in a way that doesn't get caught here, so I don't know 
	catch (...) { }
}

void CELL::UpdateCell() { table->UpdateCell(position); }		// Call update cell on GUI base pointer.

void TEXT_CELL::InitializeCell() {
	CELL::InitializeCell();
	if (displayValue[0] == L'\'') { displayValue.erase(0, 1); }		// Omit preceeding ' if it was added to enforce a text cell
}

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
	// Manually check for alpha characters since std::stod() is more forgiving than is appropriate for this situation.
	try { 
		for (auto c : rawReader()) { if (isalpha(c)) { throw invalid_argument("Error parsing input text. \nText could not be interpreted as a number"); } }
		storedValue = stod(rawReader());
	}
	catch (...) { NewCell(position, "'" + rawReader()); }
}

// I'm not sure how best to implement this mapping of text to function objects
shared_ptr<FUNCTION_CELL::FUNCTION> MatchNameToFunction(const string& inputText) {
	//return functionNameMap.find(inputText)->second;
	//return make_shared<FUNCTION_CELL::SUM>();
	
	if (inputText == "SUM") { return make_shared<FUNCTION_CELL::SUM>(); }
	else if (inputText == "AVERAGE") { return make_shared<FUNCTION_CELL::AVERAGE>(); }
	else if (inputText == "PRODUCT") { return make_shared<FUNCTION_CELL::PRODUCT>(); }
	else if (inputText == "INVERSE") { return make_shared<FUNCTION_CELL::INVERSE>(); }
	else if (inputText == "RECIPROCAL") { return make_shared<FUNCTION_CELL::RECIPROCAL>(); }
	else if (inputText == "PI") { return make_shared<FUNCTION_CELL::PI>(); }
	else { return make_shared<FUNCTION_CELL::FUNCTION>(); }
	
}

// As I write this, I realize how complicated this parsing can become.
// This approach may get overly cumbersome once I account for operators (+,-,*,/) as well as ordering parentheses.
// I have seen other parsing solutions on a superficial level and they break down the text into "token" objects.
// I may need to rework this in a more object-oriented solution to make it easier to conceptualize the various complexities.
shared_ptr<FUNCTION_CELL::ARGUMENT> FUNCTION_CELL::ParseFunctionString(string& inputText) {
	// Clear any spaces, which will interfere with parsing
	auto n = size_t{ 0 };
	while (true) {
		n = inputText.find(L' ');
		if (n == string::npos) { break; }
		inputText.erase(n, 1);
	}

	if (isalpha(inputText[0])) { /*Convert function name*/ 
		n = 0; //auto n = size_t{ 0 };
		while (isalpha(inputText[n])) { ++n; }
		auto funcName = inputText.substr(0, n);
		inputText.erase(0, n);

		// Create approprate function cell from function name
		// Call new parsing operation to fill out function arguments

		auto func = MatchNameToFunction(funcName); func->Arguments.clear();
		if (!ClearEnclosingChars(L'(', L')', inputText)) { throw invalid_argument("Error parsing input text. \nParentheses mismatch."); }	// Clear enclosing brackets of funciton call

		// Segment text within parentheses into segments deliniated by commas
		// Tracks count of parentheses to skip over nested function commas
		// This will fill the vector of ARGUMENTS used for function input parameters
		auto argSegments = vector<string>{ };
		auto countParentheses{ 0 }; auto n2 = size_t{ 0 };
		do {
			n = 0; n2 = 0;												// Reset indicies so each run starts fresh
			do {
				n = inputText.find_first_of(",()", n2);
				if (n == string::npos) { break; }						// End of string
				else if (inputText[n] == L'(') { ++countParentheses; }
				else if (inputText[n] == L')') { --countParentheses; }
				else if (countParentheses == 0) { continue; }			// Non-nested comma; stop before incrementing indicies
				n2 = ++n;												// Increment indicies to avoid stopping on the same character perpetually
			} while (countParentheses != 0);
			if (inputText.size() == 0) { break; }						// String fully parsed; don't push_back empty string
			argSegments.push_back(inputText.substr(0, n));
			inputText.erase(0, n + 1);
		} while (n != string::npos);

		for (auto arg : argSegments) { func->Arguments.push_back(ParseFunctionString(arg)); }	// For each segment, build it into an argument recursively
		func->Function();																		// Associate future with result of function
		return func;
	}
	else if (inputText[0] == '&') { /*Convert reference*/ 
		// Literally copied from reference cell initiation. Consider unifying into one funciton for ease of maintainance.
		auto row_index = min(inputText.find_first_of('R'), inputText.find_first_of('r'));
		auto col_index = min(inputText.find_first_of('C'), inputText.find_first_of('c'));

		auto pos = CELL::CELL_POSITION{ };

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

		//auto referencePosition = pos;
		SubscribeToCell(pos);
		if (cellMap.find(pos) == cellMap.end()) { error = true; }		// Dangling reference: set error flag. Still need to construct reference argument for future use.
		return make_shared<FUNCTION_CELL::REFERENCE_ARGUMENT>(*this, pos);
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

void FUNCTION_CELL::UpdateCell() {
	//func.val = std::future<double>();
	error = false;		// Reset error flag in case there was a prior error
	func.UpdateArgument();
	storedValue = func.val.get();
	CELL::UpdateCell();
}

void FUNCTION_CELL::FUNCTION::Function() {
	auto p = promise<double>{};
	val = p.get_future();
	if (Arguments.size() == 0) { error = true; return; }
	auto x = *Arguments.begin();
	p.set_value(x->val.get());		// By default, assume a single argument and simply grab its value
}

void FUNCTION_CELL::FUNCTION::UpdateArgument() {
	for (auto arg : Arguments) { arg->UpdateArgument(); }	// Update each argument before recalculating function
	Function();
}

FUNCTION_CELL::VALUE_ARGUMENT::VALUE_ARGUMENT(double arg): storedArgument(arg) {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(arg);
}

void FUNCTION_CELL::VALUE_ARGUMENT::UpdateArgument() {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(storedArgument);
}

// May throw
// Look up value upon creation
FUNCTION_CELL::REFERENCE_ARGUMENT::REFERENCE_ARGUMENT(FUNCTION_CELL& parentCell, CELL_POSITION pos) : parentCell(&parentCell), referencePosition(pos) {
	auto p = promise<double>{};
	val = p.get_future();
	try { 
		auto x = cellMap.at(pos)->DisplayOutput();		// 
		p.set_value(stod(x));	// Stored value may need to be tracked separately from display value eventually
	}
	catch (...) { }
}

void FUNCTION_CELL::REFERENCE_ARGUMENT::UpdateArgument() {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(stod(cellMap.at(referencePosition)->DisplayOutput()));	// Stored value may need to be tracked separately from display value eventually
}

void FUNCTION_CELL::SUM::Function() {
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred, [&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->val.get(); }  return sum; });
}

void FUNCTION_CELL::AVERAGE::Function() {
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred, [&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->val.get(); }  return sum / input.size(); });
}

void FUNCTION_CELL::PRODUCT::Function() {
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	double product{ 1 };
	for (auto x : Arguments) { product *= (*x).val.get(); }
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(product);
}

void FUNCTION_CELL::INVERSE::Function() {
	if (Arguments.size() != 1) { throw invalid_argument("Error parsing input text.\nExactly one argument must be given."); }
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value((*Arguments.begin())->val.get() * (-1));
}

void FUNCTION_CELL::RECIPROCAL::Function() {
	if (Arguments.size() != 1) { throw invalid_argument("Error parsing input text.\nExactly one argument must be given."); }
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(1 / (*Arguments.begin())->val.get());
}

void FUNCTION_CELL::PI::Function() {
	if (Arguments.size() != 0) { throw invalid_argument("Error parsing input text.\nFunction takes no arguments."); }
	auto p = promise<double>{};
	val = p.get_future();
	p.set_value(3.14159);		// <== May consider other ways to call/represent this number.
}