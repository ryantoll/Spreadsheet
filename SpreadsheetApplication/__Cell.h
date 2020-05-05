/*///////////////////////////////////////////////////////////////////////////////////////////////
// This file is a placeholder to show my current focus of study.
// I am presently working on learning the Gang of Four design principles for Object-Oriented code.
// To practice these principles, I plan to create a spreadsheet application.
// Below is a rough draft of the header file defining cells in the spreadsheet.
// Inheritance will allow polymorphism of cells all held in the same container.
// A factory function will also be used to create new cells based upon the user input string.
// The end goal is to create clear, flexible code open to further extention.
*////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CELL_CLASS
#define __CELL_CLASS

#include "stdafx.h"
#include <map>
#include <string>
#include <algorithm>
#include <numeric>
#include <future>
using namespace std;

// Base class for all cells.
// It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
public:
	struct CELL_POSITION {
		unsigned int row{ 0 };
		unsigned int column{ 0 };
	};

	// Internal "Singleton" factory, which has implicit access to private and protected CELL members.
	// NewCell() member function should be fixed for consistency with existing cell types.
	// Factory should be open to extention to support new cell types, defaulting to NewCell().
protected:
	class CELL_FACTORY {
	public:
		virtual ~CELL_FACTORY() {}
		static shared_ptr<CELL> NewCell(CELL_POSITION, wstring);
		static void NotifyAll(CELL_POSITION);
	};

public:
	static CELL_FACTORY cell_factory;

private:
	wstring rawContent;
	//static multimap<CELL_POSITION, CELL_POSITION> subscriptionMap;		//<Subject, Observers>
protected:
	wstring displayValue;
	bool error{ false };
	CELL_POSITION position;

	CELL() {}

	void SubscribeToCell(CELL_POSITION subject);
	void UnsubscribeFromCell(CELL_POSITION);

	// Sub-classes have read-only access to raw cell content.
	// Any change in raw content should create/initialize a new CELL through the factory funciton.
	// This ensures encapsulation so that client does not mismanage cell-type changes.
	wstring rawReader() { return rawContent; }
public:
	virtual ~CELL() {}
	virtual wstring DisplayOutput() { return error ? L"!ERROR!" : displayValue; }
	virtual wstring DisplayRawContent() { return rawContent; }
	virtual void InitializeCell() { displayValue = rawContent; }
	virtual bool MoveCell(CELL_POSITION newPosition);
	virtual void UpdateCell();		// Invoked whenever a cell needs to update it's output.
};

inline bool operator< (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) {
	if (lhs.column < rhs.column) { return true; }
	else if (lhs.column == rhs.column && lhs.row < rhs.row) { return true; }
	else { return false; }
}

inline bool operator== (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) {
	if (lhs.column == rhs.column && lhs.row == rhs.row) { return true; }
	else { return false; }
}

inline bool operator!= (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) {
	return !(lhs == rhs);
}

// Map holds all non-null cells.
// The use of an associative container rather than a sequential container obviates the need for filler cells in empty positions.
// CELL_POSITION defines it's own operator< and operator== for use in map sorting.
// The choice of column sorting preempting row sorting is arbitrary. Either way is fine so long as it is consistent.
// Note that an "Observer" pattern may be needed in association with cellMap to notify relevant cells when changes occur.
// Changes may cascade, so notifications may need to specify what changes or which cells are potentially affected.
inline map<CELL::CELL_POSITION, shared_ptr<CELL>> cellMap;

// A cell that is simply raw text merely outputs its text.
class TEXT_CELL : public CELL {
public:
	virtual ~TEXT_CELL() {}
	wstring DisplayOutput() override { return displayValue; }	// Presumably this will never be in an error state.
	void InitializeCell() override;
};

// A cell that refers to another cell by referring to it's position.
class REFERENCE_CELL : public CELL {
public:
	//REFERENCE_CELL() = default;
	virtual ~REFERENCE_CELL() { UnsubscribeFromCell(referencePosition); }
	wstring DisplayOutput() override {
		wstring out;
		auto itCell = cellMap.find(referencePosition);
		if (itCell == cellMap.end()) { out = L"!REF!"; }
		else { out = itCell->second->DisplayOutput(); }
		return error ? L"!ERROR!" : out;
		//return error ? L"!ERROR!" : cellMap[referencePosition]->DisplayOutput();
	}
	void InitializeCell() override;
protected:
	CELL_POSITION referencePosition;
};

// A base class for all cells that contains numbers.
class NUMERICAL_CELL : public CELL {
protected:
	double storedValue{ 0 };
	//DISPLAY_PARAMETERS parameters;		// Add criteria for textual representation of value. (Ex. 1 vs. 1.0000 vs. $1.00, etc.)
public:
	virtual ~NUMERICAL_CELL() {}
	wstring DisplayOutput() override { return error ? L"!ERROR!" : std::to_wstring(storedValue); }
	void InitializeCell() override;
};

// A cell that contains one or more FUNCTION(s).
// FUNCTION will utilize the "Strategy" pattern to support numerous function types by specializing the base FUNCTION type for each function used.
// FUNCTION_CELL will utilize the "Composite" pattern to treat singular and aggregate FUNCTIONS uniformly.
// Alternatively, FUNCTION could simply store a function pointer that stores the appropriate function at runtime based upon function name lookup.
// Each function must have the same call signature to fit in the same pointer object (taking a vector of ARGUMENTS and storing a double).
// The sub-classing option is used here to comport with the broader goal of demonstrating established Object-oriented design patterns.
// This alternate strategy is shown in comments for completeness.
class FUNCTION_CELL : public NUMERICAL_CELL {
public:
	virtual ~FUNCTION_CELL() {}
	void InitializeCell() override;
	void UpdateCell() override;		// Need to add recalculate logic here for when reference cells update

	struct ARGUMENT { 
		future<double> val;
		virtual void UpdateArgument() { }				// Logic to update argument when dependent cells update. May be trivial.
	};

	struct FUNCTION: public ARGUMENT {
		vector<shared_ptr<ARGUMENT>> Arguments;
		//double (*funPTR) (vector<ARGUMENT>);			// Alternate to subclassing, just assign a function to this pointer at runtime.
		virtual void Function();						// Each sub-class overrides this function to implement its own functionallity
		void UpdateArgument() override;
		bool calculationComplete{ false };
		bool error{ false };
	};

	struct VALUE_ARGUMENT : public ARGUMENT {
		double storedArgument{ };
		explicit VALUE_ARGUMENT(double);
		void UpdateArgument() override;
	};

	struct REFERENCE_ARGUMENT : public ARGUMENT {
		REFERENCE_ARGUMENT(CELL_POSITION pos);
		CELL_POSITION referencePosition;
		void UpdateArgument() override;
	};

protected:
	FUNCTION func;

	shared_ptr<FUNCTION_CELL::ARGUMENT> ParseFunctionString(wstring&);
public:
	/*////////////////////////////////////////////////////////////
	// List of available operations overriding Function
	*/////////////////////////////////////////////////////////////
	struct SUM : public FUNCTION { void Function() override; };
	struct AVERAGE : public FUNCTION { void Function() override; };
	struct PRODUCT : public FUNCTION { void Function() override; };
	struct INVERSE : public FUNCTION { void Function() override; };
	struct RECIPROCAL : public FUNCTION { void Function() override; };
	struct PI : public FUNCTION { void Function() override; };
};

#endif // !__CELL_CLASS