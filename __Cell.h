/*///////////////////////////////////////////////////////////////////////////////////////////////
//This file is a placeholder to show my current focus of study.
//I am presently working on learning the Gang of Four design principles for Object-Oriented code.
//To practice these principles, I plan to create a spreadsheet application.
//Below is a rough draft of the header file defining cells in the spreadsheet.
//Inheritance will allow polymorphism of cells all held in the same container.
//A factory function will also be used to create new cells based upon the user input string.
//The end goal is to create clear, flexible code open to further extention.
*////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CELL_CLASS
#define __CELL_CLASS

#include "stdafx.h"
#include <map>
#include <string>
using namespace std;

//Base class for all cells.
//It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
public:
	struct CELL_POSITION {
		unsigned int row{ 0 };
		unsigned int column{ 0 };
	};

	//Internal "Singleton" factory, which has implicit access to private and protected CELL members.
	//NewCell() member function should be fixed for consistency with existing cell types.
	//Factory should be open to extention to support new cell types, defaulting to NewCell().
	class CELL_FACTORY {
	public:
		virtual ~CELL_FACTORY() {}
		static shared_ptr<CELL> NewCell(CELL_POSITION, wstring);
	};

	static CELL_FACTORY cell_factory;

private:
	wstring rawContent;
protected:
	wstring displayValue;
	bool error{ false };
	CELL_POSITION position;

	CELL() {}

	//Sub-classes have read-only access to raw cell content.
	//Any change in raw content should create/initialize a new CELL through the factory funciton.
	//This ensures encapsulation so that client does not mismanage cell-type changes.
	wstring rawReader() { return rawContent; }
public:
	virtual ~CELL() {}
	virtual wstring DisplayOutput() { return error ? L"!ERROR!" : displayValue; }
	virtual void InitializeCell() { displayValue = rawContent; }
	virtual void MoveCell(CELL_POSITION newPosition);
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

//Map holds all non-null cells.
//The use of an associative container rather than a sequential container obviates the need for filler cells in empty positions.
//CELL_POSITION defines it's own operator< and operator== for use in map sorting.
//The choice of column sorting preempting row sorting is arbitrary. Either way is fine so long as it is consistent.
//Note that an "Observer" pattern may be needed in association with cellMap to notify relevant cells when changes occur.
//Changes may cascade, so notifications may need to specify what changes or which cells are potentially affected.
extern map<CELL::CELL_POSITION, shared_ptr<CELL>> cellMap;

//A cell that is simply raw text merely outputs its text.
class TEXT_CELL : public CELL {
public:
	virtual ~TEXT_CELL() {}
	wstring DisplayOutput() override { return displayValue; }	//Presumably this will never be in an error state.
};

//A cell that refers to another cell by referring to it's position.
class REFERENCE_CELL : public CELL {
public:
	//REFERENCE_CELL() = default;
	virtual ~REFERENCE_CELL() {}
	wstring DisplayOutput() override { return error ? L"!ERROR!" : cellMap[referencePosition]->DisplayOutput(); }
	void InitializeCell() override;
protected:
	CELL_POSITION referencePosition;
};

//A base class for all cells that contains numbers.
class NUMERICAL_CELL : public CELL {
	double value;
	//DISPLAY_PARAMETERS parameters;		//Add criteria for textual representation of value.
public:
	virtual ~NUMERICAL_CELL() {}
	wstring DisplayOutput() override { return std::to_wstring(value); }
	void InitializeCell() override;
};

//A cell that contains one or more FUNCTION.
//FUNCTION will utilize the "Strategy" pattern to support numerous function types.
//FUNCTION_CELL will utilize the "Composite" pattern to treat singular and aggregate FUNCTIONS uniformly.
/*class FUNCTION_CELL : public NUMERICAL_CELL {
protected:

public:
	virtual ~FUNCTION_CELL() {}

};*/

#endif // !__CELL_CLASS