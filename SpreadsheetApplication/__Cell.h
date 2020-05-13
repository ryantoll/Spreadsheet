/*///////////////////////////////////////////////////////////////////////////////////////////////
// I am presently working on learning the Gang of Four design principles for Object-Oriented code.
// To practice these principles, I have created a spreadsheet application.
// Below is a header file defining cells in the spreadsheet.
// Inheritance allows polymorphism of cells all held in the same container.
// A factory function is used to create new cells based upon the user input string.
// The end goal is to create clear, flexible code open to further extention.
*////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CELL_CLASS
#define __CELL_CLASS

#include "stdafx.h"

// Base class for all cells.
// It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
public:
	struct CELL_POSITION {
		unsigned int row{ 0 };
		unsigned int column{ 0 };
	};

	// Any changes to a CELL should trigger a notification
	// Utilizing a "Proxy" pattern ensures that a notification is sent whenever a change is made
	// Users of CELL have only indirect access to CELLs since they should not be responsible for sending notifications.
	class CELL_PROXY {
		friend class CELL;
		std::shared_ptr<CELL> cell;
		auto operator*() const noexcept { return *cell; }
	public:
		CELL_PROXY() = default;
		explicit CELL_PROXY(std::shared_ptr<CELL> target) : cell(target) { }
		~CELL_PROXY() = default;

		// Copy & move
		CELL_PROXY& operator=(std::shared_ptr<CELL> target) noexcept { cell = target; if (cell) { cell->UpdateCell(); } return *this; }
		CELL_PROXY(const CELL_PROXY& target) noexcept : cell(target.cell) { cell = target.cell; if (cell) { cell->UpdateCell(); } }
		CELL_PROXY& operator=(std::shared_ptr<CELL>&& target) noexcept { cell = target; if (cell) { cell->UpdateCell(); } return *this; }
		CELL_PROXY(const CELL_PROXY&& target) noexcept : cell(target.cell) { cell = target.cell; if (cell) { cell->UpdateCell(); } }

		auto operator->() const noexcept { return cell; }
		explicit operator bool() const noexcept { return bool{ cell }; }
	};

private:
	// Internal "Singleton" factory, which has implicit access to private and protected CELL members.
	// NewCell() member function should be fixed for consistency with existing cell types.
	// Factory should be open to extention to support new cell types, defaulting to NewCell().
	class CELL_FACTORY {
	public:
		static CELL_PROXY NewCell(CELL_POSITION, const std::string&);
		static void NotifyAll(CELL_POSITION);
	};

	// An "Observer" pattern is used to notify relevant cells when changes occur.
	// Changes may cascade, so notifications need to handle that effectively.
	// Stores a set of observers for each subject.
	static std::map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>> subscriptionMap;		// <Subject, (set of) Observers>

	// Map holds all non-null cells.
	// The use of an associative container rather than a sequential container obviates the need for filler cells in empty positions.
	// CELL_POSITION defines it's own operator< and operator== for use in map sorting.
	// The choice of column sorting preempting row sorting is arbitrary. Either way is fine so long as it is consistent.
	static std::map<CELL::CELL_POSITION, std::shared_ptr<CELL>> cellMap;
	// inline std::unordered_map<CELL::CELL_POSITION, std::string> cellMap;	// Consider hash map as an alternative
	static std::mutex lkSubMap, lkCellMap;
public:
	static CELL_FACTORY cell_factory;
	static CELL_PROXY GetCellProxy(CELL::CELL_POSITION);
private:
	CELL(CELL_PROXY cell) { *this = *cell; CELL_FACTORY::NotifyAll(position); }		// Create cell from cell proxy and notify of change

	std::string rawContent;
	static std::shared_ptr<CELL> GetCell(CELL::CELL_POSITION);
protected:
	CELL() {}
	std::string displayValue;
	bool error{ false };
	CELL_POSITION position;

	void SubscribeToCell(CELL_POSITION subject) const;
	void UnsubscribeFromCell(CELL_POSITION) const;
	static void UnsubscribeFromCell(CELL_POSITION, CELL_POSITION);

	// Sub-classes have read-only access to raw cell content.
	// Any change in raw content should create/initialize a new CELL through the factory funciton.
	// This ensures encapsulation so that client does not mismanage cell-type changes.
	std::string rawReader() const { return rawContent; }
public:
	virtual ~CELL() {}
	virtual std::string DisplayOutput() const { return error ? "!ERROR!" : displayValue; }
	virtual std::string DisplayRawContent() const { return rawContent; }
	virtual void InitializeCell() { displayValue = rawContent; }
	virtual bool MoveCell(CELL_POSITION newPosition);
	virtual void UpdateCell();		// Invoked whenever a cell needs to update it's output.
	CELL_POSITION GetPosition() { return position; }
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

// A cell that is simply raw text merely outputs its text.
class TEXT_CELL : public CELL {
public:
	virtual ~TEXT_CELL() {}
	std::string DisplayOutput() const override { return displayValue; }		// Presumably this will never be in an error state.
	void InitializeCell() override;
};

// A cell that refers to another cell by referring to it's position.
class REFERENCE_CELL : public CELL {
public:
	virtual ~REFERENCE_CELL() { UnsubscribeFromCell(referencePosition); }
	std::string DisplayOutput() const override {
		auto out = std::string{ };
		auto cell = GetCellProxy(referencePosition);
		if (!cell || cell->GetPosition() == position) { out = "!REF!"; }	// Dangling reference & reference to self both cause a reference error.
		else { out = cell->DisplayOutput(); }
		return error ? "!ERROR!" : out;
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
	std::string DisplayOutput() const override { return error ? "!ERROR!" : std::to_string(storedValue); }
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
	void InitializeCell() override;
	void UpdateCell() override;		// Need to add recalculate logic here for when reference cells update

	struct ARGUMENT { 
		std::future<double> val;
		virtual void UpdateArgument() { }				// Logic to update argument when dependent cells update. May be trivial.
	};

	struct FUNCTION: public ARGUMENT {
		std::vector<std::shared_ptr<ARGUMENT>> Arguments;
		//double (*funPTR) (vector<ARGUMENT>);			// Alternate to subclassing, just assign a function to this pointer at runtime.
		virtual void Function();						// Each sub-class overrides this function to implement its own functionallity
		void UpdateArgument() override;
		bool error{ false };
	};

	struct VALUE_ARGUMENT : public ARGUMENT {
		double storedArgument{ };
		explicit VALUE_ARGUMENT(double);
		void UpdateArgument() override;
	};

	struct REFERENCE_ARGUMENT : public ARGUMENT {
		REFERENCE_ARGUMENT(FUNCTION_CELL&, CELL_POSITION);
		~REFERENCE_ARGUMENT() { UnsubscribeFromCell(referencePosition, parentPosition); }
		CELL_POSITION referencePosition, parentPosition;
		void UpdateArgument() override;
	};

protected:
	FUNCTION func;
	
	std::shared_ptr<FUNCTION_CELL::ARGUMENT> ParseFunctionString(std::string&);
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