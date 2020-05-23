/*//////////
// This file seperates out the Function aspect of cells from the cell aspect of behavior
// 
*///////////

#include "stdafx.h"
#include "__Cell.h"
#include "Utilities.h"

using namespace std;
using namespace RYANS_UTILITIES;

REFERENCE_ARGUMENT::~REFERENCE_ARGUMENT() { parentContainer->UnsubscribeFromCell(referencePosition, parentPosition); }

shared_ptr<FUNCTION> MatchNameToFunction(const string& inputText, vector<shared_ptr<ARGUMENT>>&& args) {
	if (inputText == "SUM"s) { return make_shared<SUM>(std::move(args)); }
	else if (inputText == "AVERAGE"s) { return make_shared<AVERAGE>(std::move(args)); }
	else if (inputText == "PRODUCT"s) { return make_shared<PRODUCT>(std::move(args)); }
	else if (inputText == "INVERSE"s) { return make_shared<INVERSE>(std::move(args)); }
	else if (inputText == "RECIPROCAL"s) { return make_shared<RECIPROCAL>(std::move(args)); }
	else if (inputText == "PI"s) { return make_shared<PI>(); }
	else { return make_shared<FUNCTION>(std::move(args)); }
}

// As I write this, I realize how complicated this parsing can become.
// This approach may get overly cumbersome once I account for operators (+,-,*,/) as well as ordering parentheses.
// I have seen other parsing solutions on a superficial level and they break down the text into "token" objects.
// I may need to rework this in a more object-oriented solution to make it easier to conceptualize the various complexities.
shared_ptr<ARGUMENT> FUNCTION_CELL::ParseFunctionString(string& inputText) {
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

		auto vArgs = vector<shared_ptr<ARGUMENT>>{ };
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
		return make_shared<REFERENCE_ARGUMENT>(parentContainer, *this, pos);
	}
	else if (isdigit(inputText[0]) || inputText[0] == '.' || inputText[0] == '-') { /*Convert to value*/
		while (isdigit(inputText[n]) || inputText[n] == '.' || inputText[n] == '-') { ++n; }	// Keep grabbing chars until an invalid char is reached
		auto num = inputText.substr(0, n);
		inputText.erase(0, n);
		return make_shared<VALUE_ARGUMENT>(stod(num));	// wstring -> double -> Value Argument
	}
	else { throw invalid_argument("Error parsing input text."); }	/*Set error flag*/
}

// Set the value of the associated future.
void ARGUMENT::SetValue(double value) noexcept {
	auto p = promise<double>{ };
	val = p.get_future();
	p.set_value(value);
}

// Set an exception for the associated future.
void ARGUMENT::SetValue(std::exception error) noexcept {
	auto pError = make_exception_ptr(error);
	auto p = promise<double>{};
	val = p.get_future();
	p.set_exception(pError);
}

FUNCTION::FUNCTION(vector<shared_ptr<ARGUMENT>>&& args) : Arguments{ std::move(args) } {
	if (Arguments.size() == 0) { error = true; return; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; return; }
}

// Update FUNCTION by first updating all arguments, then setting the future again.
bool FUNCTION::UpdateArgument() noexcept {
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (Arguments.size() == 0) { error = true; stillValid = false; return !stillValid; }
	try { SetValue((*Arguments.begin())->Get()); }
	catch (...) { error = true; }
	return !stillValid;
}

// A single value merely needs to set the associated future with the given value.
VALUE_ARGUMENT::VALUE_ARGUMENT(double arg) { storedArgument = arg; SetValue(arg); }

// Ditto for updating the argument.
bool VALUE_ARGUMENT::UpdateArgument() noexcept { SetValue(storedArgument); return false; }

// Reference arugment stores positions of target and parent cells and then updates it's argument.
REFERENCE_ARGUMENT::REFERENCE_ARGUMENT(CELL::CELL_DATA* container, FUNCTION_CELL& parentCell, CELL::CELL_POSITION pos)
	: parentContainer(container), referencePosition(pos), parentPosition(parentCell.GetPosition()) {
	UpdateArgument();
}

// Look up referenced value and store in the associated future.
// Store an exception if there's a dangling or circular reference.
bool REFERENCE_ARGUMENT::UpdateArgument() noexcept {
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

SUM::SUM(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

AVERAGE::AVERAGE(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

PRODUCT::PRODUCT(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

INVERSE::INVERSE(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

RECIPROCAL::RECIPROCAL(vector<shared_ptr<ARGUMENT>>&& args) { Arguments = std::move(args); UpdateArgument(); }

PI::PI() { UpdateArgument(); }

bool SUM::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		auto& input = Arguments;
		val = async(std::launch::async | std::launch::deferred,
			[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum; });
	}
	return !stillValid;
}

bool AVERAGE::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		auto& input = Arguments;
		val = async(std::launch::async | std::launch::deferred,
			[&input] { auto sum = 0.0; for (auto i = input.begin(); i != input.end(); ++i) { sum += (*i)->Get(); }  return sum / input.size(); });
	}
	return !stillValid;
}

bool PRODUCT::UpdateArgument() noexcept {
	if (Arguments.size() == 0) { SetValue(invalid_argument{ "Error parsing input text.\nNo arguments provided." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		double product{ 1 };
		try { for (auto x : Arguments) { product *= (*x).Get(); } SetValue(product); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool INVERSE::UpdateArgument() noexcept {
	if (Arguments.size() != 1) { SetValue(invalid_argument{ "Error parsing input text.\nExactly one argument must be given." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		try { SetValue(((*Arguments.begin())->Get()) * (-1)); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool RECIPROCAL::UpdateArgument() noexcept {
	if (Arguments.size() != 1) { SetValue(invalid_argument{ "Error parsing input text.\nExactly one argument must be given." }); }
	for (auto arg : Arguments) { if (arg->UpdateArgument()) { stillValid = false; } }
	if (!stillValid) {
		try { SetValue(1 / (*Arguments.begin())->Get()); }
		catch (exception error) { SetValue(error); }
	}
	return !stillValid;
}

bool PI::UpdateArgument() noexcept {
	if (Arguments.size() != 0) { SetValue(invalid_argument{ "Error parsing input text.\nFunction takes no arguments." }); }
	auto pi = 3.14159;
	Arguments.push_back(make_shared<VALUE_ARGUMENT>(pi));
	SetValue(pi);	// <== May consider other ways to call/represent this number.
	return !stillValid;
}