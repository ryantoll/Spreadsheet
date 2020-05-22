#include "stdafx.h"
#include "Utilities.h"
#include "__Cell.h"
#include "__Table.h"

using namespace std;
using namespace RYANS_UTILITIES;

CELL::CELL_PROXY CELL::CELL_FACTORY::NewCell(CELL_DATA* parentContainer, const CELL_POSITION position, const string& contents) noexcept {
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

void CELL::CELL_FACTORY::RecreateCell(CELL_DATA* parentContainer, const CELL_PROXY& cell, const CELL_POSITION pos) noexcept {
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
		if (!oCell) { continue; }
		oCell->UpdateCell();
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
	table->UpdateCell(position); 		// Call update cell on GUI base pointer.
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
	referencePosition = ReferenceStringToCellPosition(GetRawContent());
	SubscribeToCell(referencePosition);
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
	catch (...) { CELL::CELL_FACTORY::NewCell(parentContainer, position, "'" + GetRawContent()); }
}

// I'm not sure how best to implement this mapping of text to function objects.
// Each one must get a newly created object, lest they share the same arguments.
shared_ptr<FUNCTION_CELL::FUNCTION> MatchNameToFunction(const string& inputText, vector<shared_ptr<FUNCTION_CELL::ARGUMENT>>&& args) {	
	if (inputText == "SUM"s) { return make_shared<FUNCTION_CELL::SUM>( std::move(args) ); }
	else if (inputText == "AVERAGE"s) { return make_shared<FUNCTION_CELL::AVERAGE>( std::move(args) ); }
	else if (inputText == "PRODUCT"s) { return make_shared<FUNCTION_CELL::PRODUCT>( std::move(args) ); }
	else if (inputText == "INVERSE"s) { return make_shared<FUNCTION_CELL::INVERSE> (std::move(args) ); }
	else if (inputText == "RECIPROCAL"s) { return make_shared<FUNCTION_CELL::RECIPROCAL>( std::move(args) ); }
	else if (inputText == "PI"s) { return make_shared<FUNCTION_CELL::PI>(); }
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
				n = inputText.find_first_of(",()"s, n2);
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
		if (!parentContainer->GetCellProxy(pos)) { error = true; }		// Dangling reference: set error flag. Still need to construct reference argument for future use.
		return make_shared<FUNCTION_CELL::REFERENCE_ARGUMENT>(parentContainer, *this, pos);
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
void FUNCTION_CELL::InitializeCell() noexcept {
	auto inputText = GetRawContent().substr(1);
	auto vArgs = vector<shared_ptr<ARGUMENT>>{ };
	vArgs.push_back(ParseFunctionString(inputText));	// Recursively parse input string
	func = make_shared<FUNCTION>( std::move(vArgs) );
	storedValue = func->Get();
}

// Recalculate function when an underlying reference argument is changed.
void FUNCTION_CELL::UpdateCell() noexcept {
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
void FUNCTION_CELL::ARGUMENT::SetValue(std::exception error) noexcept {
	auto pError = make_exception_ptr(error);
	auto p = promise<double>{};
	val = p.get_future();
	p.set_exception(pError);
}

FUNCTION_CELL::FUNCTION::FUNCTION(vector<shared_ptr<ARGUMENT>>&& args) : Arguments{ std::move(args) } {
	if (Arguments.size() == 0) { error = true; return; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; return; }
}

// Update FUNCTION by first updating all arguments, then setting the future again.
bool FUNCTION_CELL::FUNCTION::UpdateArgument() noexcept { 
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (Arguments.size() == 0) { error = true; stillValid = false; return !stillValid; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; }
	return !stillValid;
}

// A single value merely needs to set the associated future with the given value.
FUNCTION_CELL::VALUE_ARGUMENT::VALUE_ARGUMENT(double arg) { storedArgument = arg; SetValue(arg); }

// Ditto for updating the argument.
bool FUNCTION_CELL::VALUE_ARGUMENT::UpdateArgument() noexcept { SetValue(storedArgument); return false; }

// Reference arugment stores positions of target and parent cells and then updates it's argument.
FUNCTION_CELL::REFERENCE_ARGUMENT::REFERENCE_ARGUMENT(CELL::CELL_DATA* container, FUNCTION_CELL& parentCell, CELL_POSITION pos) 
	: parentContainer(container), referencePosition(pos), parentPosition(parentCell.position) {	UpdateArgument(); }

// Look up referenced value and store in the associated future.
// Store an exception if there's a dangling or circular reference.
bool FUNCTION_CELL::REFERENCE_ARGUMENT::UpdateArgument() noexcept {
	auto refCell = parentContainer->GetCellProxy(referencePosition);
	try {
		if (!refCell || refCell->GetPosition() == parentPosition) { throw invalid_argument{ "Reference Error" }; }	// Check that value exists and is not circular reference
		auto nValue = stod(refCell->GetRawContent());
		SetValue(nValue);	// Stored value may need to be tracked separately from display value eventually
		nValue == storedArgument ? stillValid = true : stillValid = false;
	}
	catch (std::exception error) { SetValue(error); }
	return !stillValid;
}

/*////////////////////////////////////////////////////////////////////////////////////////////////////
// Procedures for supported FUNCTION_CELL::FUNCTIONs
// Each procedure needs to move in the argument vector and call UpdateArguments()
// Each update needs to update all of its arguments recursively
// Function needs to be updated for each read since its old future is no longer valid once read
*/////////////////////////////////////////////////////////////////////////////////////////////////////

FUNCTION_CELL::SUM::SUM(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

FUNCTION_CELL::AVERAGE::AVERAGE(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

FUNCTION_CELL::PRODUCT::PRODUCT(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

FUNCTION_CELL::INVERSE::INVERSE(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

FUNCTION_CELL::RECIPROCAL::RECIPROCAL(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

FUNCTION_CELL::PI::PI() { UpdateArgument(); }

bool FUNCTION_CELL::SUM::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		auto& input = Arguments;
		val = async(std::launch::async | std::launch::deferred,
			[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum; });
	}
	return !stillValid;
}

bool FUNCTION_CELL::AVERAGE::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		auto& input = Arguments;
		val = async(std::launch::async | std::launch::deferred,
			[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum / input.size(); });
	}
	return !stillValid;
}

bool FUNCTION_CELL::PRODUCT::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		double product{ 1 };
		try { for (auto x : Arguments) { product *= (*x).Get(); } SetValue(product); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool FUNCTION_CELL::INVERSE::UpdateArgument() noexcept {
	if (Arguments.size() != 1) { SetValue(invalid_argument{ "Error parsing input text.\nExactly one argument must be given." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		try { SetValue(((*Arguments.begin())->Get()) * (-1)); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool FUNCTION_CELL::RECIPROCAL::UpdateArgument() noexcept {
	if (Arguments.size() != 1) { SetValue(invalid_argument{ "Error parsing input text.\nExactly one argument must be given." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		try { SetValue(1 / (*Arguments.begin())->Get()); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool FUNCTION_CELL::PI::UpdateArgument() noexcept {
	if (Arguments.size() != 0) { SetValue(invalid_argument{ "Error parsing input text.\nFunction takes no arguments." }); }
	auto pi = 3.14159;
	Arguments.push_back(make_shared<VALUE_ARGUMENT>( pi ));
	SetValue(pi);	// <== May consider other ways to call/represent this number.
	return !stillValid;
}