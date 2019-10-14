/*///////////////////////////////////////////////////////////////////////////////////////////////
//This file is a placeholder to show my current focus of study.
//I am presently working on learning the Gang of Four design principles for Object-Oriented code.
//To practice these principles, I plan to create a spreadsheet application.
//Below is a rough draft of the header file defining cells in the spreadsheet.
//Inheritance will allow polymorphism of cells all held in the same container.
//A factory function will also be used to create new cells based upon the user input string.
//The end goal is to create clear, flexible code open to further extention.
*////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "stdafx.h"
#include <string>
using std::wstring;
using std::shared_ptr;

typedef int COLUMN_INDEX;
typedef int ROW_INDEX;

//Base class for all cells.
//It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
protected:
	wstring rawContent;
	wstring displayValue;

	COLUMN_INDEX col;
	ROW_INDEX row;

	CELL() {}

	//Add in-class factory builder function
	class CELL_FACTORY {
	public:
		enum class CELL_TYPE {
			TEXT,
			REFERENCE,
			INTEGRAL,
			DECIMAL,
			FUNCTION
		};

		static shared_ptr<CELL> CreateCell(CELL_TYPE type) {
			auto cell = std::make_shared<CELL>();
			//Add cast here...
			return cell;
		}
	};
public:
	virtual ~CELL() {}
	virtual wstring DisplayOutput() {}
};

//A cell that is simply raw text merely outputs its text.
class TEXT_CELL : public CELL {
public:
	virtual ~TEXT_CELL() {}
	wstring DisplayOutput() override { return displayValue; }
};

//A cell that refers to another cell.
class REFERENCE_CELL : public CELL {
protected:
	CELL& cell;
public:
	virtual ~REFERENCE_CELL() {}
	wstring DisplayOutput() override { return cell.DisplayOutput(); }
};

//A base class for all cells that contains numbers.
class NUMERICAL_CELL : public CELL {
	//display digits
public:
	virtual ~NUMERICAL_CELL() {}
};

//A cell containing an integral value up to 32 bits.
class INTEGRAL_CELL : public NUMERICAL_CELL {
protected:
	long value;
public:
	virtual ~INTEGRAL_CELL() {}
	wstring DisplayOutput() override { return std::to_wstring(value); }
};

//
class DECIMAL_CELL : public NUMERICAL_CELL {
protected:
	double value;
public:
	virtual ~DECIMAL_CELL() {}
	wstring DisplayOutput() override { return std::to_wstring(value); }
};

//
class FUNCTION_CELL : public NUMERICAL_CELL {
protected:

public:
	virtual ~FUNCTION_CELL() {}

};