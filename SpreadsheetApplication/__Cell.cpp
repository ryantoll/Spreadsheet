#include "stdafx.h"
#include "Utilities.h"
#include "__Cell.h"
#include "__Table.h"

using std::string;
using std::vector;
using std::shared_ptr;
using std::promise;
using std::invalid_argument;
using std::make_shared;
using std::make_unique;
using std::lock_guard;
using std::future;
using std::mutex;
using namespace RYANS_UTILITIES;

// Initialize static data members of CELL
// !!! -- IMPORTANT -- !!!	cellMap must be initialized after subscrptionMap		!!! -- IMPORTANT -- !!!
// !!! -- IMPORTANT -- !!!	Improper ordering will cause program to crash upon exit	!!! -- IMPORTANT -- !!!
// This is due to object lifetimes. Deconstruction of CELLs in cellMap cues unsubscription of CELL observation.
// The CELL then searches through the deconstructed subscriptionMap to remove itself.
// This causes an internal noexcept function to throw since it is traversing a tree that is no longer in a valid state.
std::unordered_map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>, CELL::CELL_HASH> CELL::subscriptionMap{ };	// <Subject, Observers>
std::unordered_map<CELL::CELL_POSITION, shared_ptr<CELL>, CELL::CELL_HASH> CELL::cellMap{ };						// Stores all CELLs
// std::map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>> CELL::subscriptionMap{ };		// <Subject, Observers>
// std::map<CELL::CELL_POSITION, shared_ptr<CELL>> CELL::cellMap{ };							// Stores all CELLs
mutex CELL::lkSubMap{ }, CELL::lkCellMap{ };

// Part of an alternative function mapping scheme
//map<wstring, shared_ptr<FUNCTION_CELL::FUNCTION>> functionNameMap{ {wstring(L"SUM"), shared_ptr<FUNCTION_CELL::SUM>()}, {wstring(L"AVERAGE"), shared_ptr<FUNCTION_CELL::AVERAGE>()} };

CELL::CELL_PROXY CELL::CELL_FACTORY::NewCell(CELL_POSITION position, const string& contents) {
	// Check for valid cell position. Disallowing R == 0 && C == 0 not only fits (non-programmer) human intuition,
	// but also prevents accidental errors in failing to specify a location.
	// R == 0 || C == 0 almost certainly indicates a failure to specify one or both arguments.
	if (position.row == 0 || position.column == 0) { throw invalid_argument("Neither Row 0, nor Column 0 exist."); }

	// Empty contents argument not only fails to create a new cell, but deletes any cell that may already exist at that position.
	// Notify any observing cells about the change *AFTER* the change has occurred.
	// (Note that control flow immediately goes to any updating cells.)
	auto oldCell = GetCell(position);
	if (contents == "") { if (oldCell) { cellMap.erase(oldCell->position); NotifyAll(position); } return CELL::CELL_PROXY{ nullptr }; }

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

	{
		auto lk = lock_guard<mutex>{ lkCellMap };	// Lock map only for write operation.
		cellMap[position] = cell;					// Add cell to cell map upon creation.
	}

	try { cell->InitializeCell(); }			// Call initialize on cell.
	catch (...) { cell->error = true; }		// Failure of any sort will set the cell into an error state.
	NotifyAll(position);					// Notify any cells that may be observing this position.

	table->UpdateCell(position);			// Notify GUI to update cell value.
	return GetCellProxy(position);			// Return stored cell so that failed numerical cells return the stored fallback text cell rather than the original failed numerical cell.
}

// Notifies observing CELLs of change in underlying data.
// Each CELL is responsible for checking the new data.
void CELL::CELL_FACTORY::NotifyAll(CELL_POSITION subject) {
	auto notificationSet = std::set<CELL_POSITION>{ };
	{
		auto lk = lock_guard<mutex>{ lkSubMap };		// Lock only to get local copy of notification set
		auto it = subscriptionMap.find(subject);
		if (it == subscriptionMap.end()) { return; }
		notificationSet = it->second;					// Get local copy of notification set so that lock can be released before updating cells, which will require it's own lock downstream
	}
	for (auto observer: notificationSet) { GetCell(observer)->UpdateCell(); }
}

// Move a cell to a new location, updating its key as well to reflect the change.
bool CELL::MoveCell(CELL_POSITION newPosition) {
	if (cellMap.find(newPosition) != cellMap.end()) { return false; }
	cellMap[newPosition] = cellMap[position];
	cellMap.erase(position);
	position = newPosition;
	return true;
}

// Subscribe to notification of changes in target CELL.
void CELL::SubscribeToCell(CELL_POSITION subject) const { 
	auto lk = lock_guard<mutex>{ lkSubMap };
	auto& observerSet = subscriptionMap[subject]; 
	observerSet.insert(position);
}

// Use static overload below
void CELL::UnsubscribeFromCell(CELL_POSITION subject) const { UnsubscribeFromCell(subject, position); }

// Remove observer link
void CELL::UnsubscribeFromCell(CELL_POSITION subject, CELL_POSITION observer) {
	auto lk = lock_guard<mutex>{ lkSubMap };
	auto itSubject = subscriptionMap.find(subject);
	if (itSubject == subscriptionMap.end()) { return; }
	(*itSubject).second.erase(observer);
}

std::shared_ptr<CELL> CELL::GetCell(CELL::CELL_POSITION pos) {
	auto lk = lock_guard<mutex>{ lkCellMap };
	auto it = cellMap.find(pos);
	return it != cellMap.end() ? it->second : nullptr;
}

CELL::CELL_PROXY CELL::GetCellProxy(CELL::CELL_POSITION pos) { return CELL_PROXY{ GetCell(pos) }; }

void CELL::UpdateCell() { 
	table->UpdateCell(position); 		// Call update cell on GUI base pointer.
	CELL_FACTORY::NotifyAll(position);	// Cascade notification
}

void TEXT_CELL::InitializeCell() {
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
void REFERENCE_CELL::InitializeCell() {
	referencePosition = ReferenceStringToCellPosition(rawReader());
	SubscribeToCell(referencePosition);
}

// Override default error behavior.
// Any cell that seems like a number, but cannot be converted to such defaults to text.
// Create a new cell at the same position with a prepended text-enforcement character.
// Manually check for alpha characters since std::stod() is more forgiving than is appropriate for this situation.
void NUMERICAL_CELL::InitializeCell() {
	try { 
		for (auto c : rawReader()) { if (!isdigit(c) && c != '.' && c != '-') { throw invalid_argument("Error parsing input text. \nText could not be interpreted as a number"); } }
		storedValue = stod(rawReader());
	}
	catch (...) { CELL::cell_factory.NewCell(position, "'" + rawReader()); }
}

// I'm not sure how best to implement this mapping of text to function objects.
// Each one must get a newly created object, lest they share the same arguments.
shared_ptr<FUNCTION_CELL::FUNCTION> MatchNameToFunction(const string& inputText, vector<shared_ptr<FUNCTION_CELL::ARGUMENT>>&& args) {	
	if (inputText == "SUM") { return make_shared<FUNCTION_CELL::SUM>( std::move(args) ); }
	else if (inputText == "AVERAGE") { return make_shared<FUNCTION_CELL::AVERAGE>( std::move(args) ); }
	else if (inputText == "PRODUCT") { return make_shared<FUNCTION_CELL::PRODUCT>( std::move(args) ); }
	else if (inputText == "INVERSE") { return make_shared<FUNCTION_CELL::INVERSE> (std::move(args) ); }
	else if (inputText == "RECIPROCAL") { return make_shared<FUNCTION_CELL::RECIPROCAL>( std::move(args) ); }
	else if (inputText == "PI") { return make_shared<FUNCTION_CELL::PI>(); }
	else { return make_shared<FUNCTION_CELL::FUNCTION>( std::move(args) ); }
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
	n = 0;	// Reset n for later use.

	if (isalpha(inputText[0])) { /*Convert function name*/ 
		while (isalpha(inputText[n])) { ++n; }
		auto funcName = inputText.substr(0, n);
		inputText.erase(0, n);
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

		auto vArgs = vector<shared_ptr<ARGUMENT>> { };
		for (auto arg : argSegments) { vArgs.push_back(ParseFunctionString(arg)); }	// For each segment, build it into an argument recursively
		
		// Create approprate function cell from function name
		// Call new parsing operation to fill out function arguments
		auto func = MatchNameToFunction(funcName, std::move(vArgs));
		return func;
	}
	else if (inputText[0] == '&') { /*Convert reference*/ 
		auto pos = ReferenceStringToCellPosition(inputText);
		SubscribeToCell(pos);
		if (!CELL::GetCellProxy(pos)) { error = true; }		// Dangling reference: set error flag. Still need to construct reference argument for future use.
		return make_shared<FUNCTION_CELL::REFERENCE_ARGUMENT>(*this, pos);
	}
	else if (isdigit(inputText[0]) || inputText[0] == '.' || inputText[0] == '-') { /*Convert to value*/
		while (isdigit(inputText[n]) || inputText[n] == '.' || inputText[n] == '-') { ++n; }	// Keep grabbing chars until an invalid char is reached
		auto num = inputText.substr(0, n);
		inputText.erase(0, n);
		return make_shared<FUNCTION_CELL::VALUE_ARGUMENT>(stod(num));	// wstring -> double -> Value Argument
	}
	else { throw invalid_argument("Error parsing input text."); }	/*Set error flag*/
}

// Parse function text into actual functions.
void FUNCTION_CELL::InitializeCell() {
	auto inputText = rawReader().substr(1);
	auto vArgs = vector<shared_ptr<ARGUMENT>>{ };
	vArgs.push_back(ParseFunctionString(inputText));	// Recursively parse input string
	func = make_shared<FUNCTION>( std::move(vArgs) );
	storedValue = func->Get();
}

// Recalculate function when an underlying reference argument is changed.
void FUNCTION_CELL::UpdateCell() {
	displayValue = "";
	error = false;		// Reset error flag in case there was a prior error
	func->UpdateArgument();
	try { storedValue = func->Get(); }
	catch (...) { error = true; }
	CELL::UpdateCell();
}

// Set the value of the associated future.
void FUNCTION_CELL::ARGUMENT::SetValue(double value) noexcept {
	auto p = promise<double>{ };
	val = p.get_future();
	p.set_value(value);
}

// Set an exception for the associated future.
void FUNCTION_CELL::ARGUMENT::SetValue(std::exception_ptr error) noexcept {
	auto p = promise<double>{};
	val = p.get_future();
	p.set_exception(error);
}

FUNCTION_CELL::FUNCTION::FUNCTION(vector<shared_ptr<ARGUMENT>>&& args) : Arguments{ std::move(args) } {
	if (Arguments.size() == 0) { error = true; return; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; return; }
}

// Update FUNCTION by first updating all arguments, then setting the future again.
void FUNCTION_CELL::FUNCTION::UpdateArgument() noexcept { 
	for (auto arg : Arguments) { arg->UpdateArgument(); } 
	if (Arguments.size() == 0) { error = true; return; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; return; }
}

// A single value merely needs to set the associated future with the given value.
FUNCTION_CELL::VALUE_ARGUMENT::VALUE_ARGUMENT(double arg): storedArgument(arg) { SetValue(arg); }

// Ditto for updating the argument.
void FUNCTION_CELL::VALUE_ARGUMENT::UpdateArgument() noexcept  { SetValue(storedArgument); }

// Reference arugment stores positions of target and parent cells and then updates it's argument.
FUNCTION_CELL::REFERENCE_ARGUMENT::REFERENCE_ARGUMENT(FUNCTION_CELL& parentCell, CELL_POSITION pos) 
	: referencePosition(pos), parentPosition(parentCell.position) {	UpdateArgument(); }

// Look up referenced value and store in the associated future.
// Store an exception if there's a dangling or circular reference.
void FUNCTION_CELL::REFERENCE_ARGUMENT::UpdateArgument() noexcept {
	auto refCell = GetCellProxy(referencePosition);
	try {
		if (!refCell || refCell->GetPosition() == parentPosition) { throw invalid_argument{ "Reference Error" }; }	// Check that value exists and is not circular reference
		SetValue(stod(refCell->DisplayOutput()));	// Stored value may need to be tracked separately from display value eventually
	}
	catch (std::exception error) { SetValue(std::make_exception_ptr(error)); }
}

/*////////////////////////////////////////////////////////////
// Procedures for supported FUNCTION_CELL::FUNCTIONs
*/////////////////////////////////////////////////////////////

FUNCTION_CELL::SUM::SUM(vector<shared_ptr<ARGUMENT>>&& args) {
	Arguments = std::move(args);
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred,
		[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum; });
}

FUNCTION_CELL::AVERAGE::AVERAGE(vector<shared_ptr<ARGUMENT>>&& args) {
	Arguments = std::move(args);
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	auto& input = Arguments;
	val = async(std::launch::async | std::launch::deferred, 
		[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum / input.size(); });
}

FUNCTION_CELL::PRODUCT::PRODUCT(vector<shared_ptr<ARGUMENT>>&& args) {
	Arguments = std::move(args);
	if (Arguments.size() == 0) { throw invalid_argument("Error parsing input text.\nNo arguments provided."); }
	double product{ 1 };
	for (auto x : Arguments) { product *= (*x).Get(); }
	SetValue(product);
}

FUNCTION_CELL::INVERSE::INVERSE(vector<shared_ptr<ARGUMENT>>&& args) {
	Arguments = std::move(args);
	if (Arguments.size() != 1) { throw invalid_argument("Error parsing input text.\nExactly one argument must be given."); }
	SetValue(((*Arguments.begin())->Get()) * (-1));
}

FUNCTION_CELL::RECIPROCAL::RECIPROCAL(vector<shared_ptr<ARGUMENT>>&& args) {
	Arguments = std::move(args);
	if (Arguments.size() != 1) { throw invalid_argument("Error parsing input text.\nExactly one argument must be given."); }
	SetValue(1 / (*Arguments.begin())->Get());
}

FUNCTION_CELL::PI::PI() {
	if (Arguments.size() != 0) { throw invalid_argument("Error parsing input text.\nFunction takes no arguments."); }
	SetValue(3.14159);	// <== May consider other ways to call/represent this number.
}