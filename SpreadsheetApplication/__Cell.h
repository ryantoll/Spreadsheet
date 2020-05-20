/*///////////////////////////////////////////////////////////////////////////////////////////////
// Below is a header file defining cells in the spreadsheet.
// Inheritance allows polymorphism of cells all held in the same container.
// A factory function is used to create new cells based upon the user input string.
// Derived classes have read-only access to raw cell content.
// Any change in raw content should create/initialize a new CELL through the factory funciton.
// This ensures encapsulation so that client does not mismanage cell-type changes.
//
// The optimal choice of container for both CELL and subscription data is an open question.
// Each choice offers different tradeoffs of time/space usage and may require benchmarking to decide.
// First used was an ordered map, which could skip empty cells and offered decent operation speed O( log(n) ).
// Next, an unordered map (hash table) was used to get constant time O(1) expected operation speed.
// Also, a 2-D array could be used to get O(1) speed plus cache localization at the cost of many empty slots.
// CELL_POSITION defines it's own operator< and operator== for use in map sorting as well as a hash function.
// The choice of column sorting preempting row sorting is arbitrary. Either way is fine so long as it is consistent.
*////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CELL_CLASS_H
#define CELL_CLASS_H

#include "stdafx.h"

// Base class for all cells.
// It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
public:
	struct CELL_POSITION {
		unsigned int column{ 0 };
		unsigned int row{ 0 };
	};

	struct CELL_HASH {
		std::size_t operator() (CELL::CELL_POSITION const& pos) const noexcept {
			auto x = unsigned long long{ 0 };
			x = x | pos.column;
			x = x << 32;
			x = x | pos.row;
			return x;
		}
	};

	// Any changes to a CELL should trigger a notification
	// Utilizing a "Proxy" pattern ensures that a notification is sent whenever a change is made
	// Users of CELL have only indirect access to CELLs since they should not be responsible for sending notifications.
	class CELL_PROXY {
		friend class CELL;
		friend class CELL_FACTORY;
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
		static void RecreateCell(CELL_PROXY&, CELL_POSITION);
		static void NotifyAll(CELL_POSITION);
	};

	// An "Observer" pattern is used to notify relevant cells when changes occur.
	// Changes may cascade, so notifications need to handle that effectively.
	// Stores a set of observers for each subject.
	static std::unordered_map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>, CELL_HASH> subscriptionMap;	// <Subject, (set of) Observers>
	static std::unordered_map<CELL::CELL_POSITION, std::shared_ptr<CELL>, CELL_HASH> cellMap;					// Cell data
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

// Initialize static data members of CELL
// !!! -- IMPORTANT -- !!!	cellMap must be initialized after subscrptionMap		!!! -- IMPORTANT -- !!!
// !!! -- IMPORTANT -- !!!	Improper ordering will cause program to crash upon exit	!!! -- IMPORTANT -- !!!
// This is due to object lifetimes. Deconstruction of CELLs in cellMap cues unsubscription of CELL observation.
// The CELL then searches through the deconstructed subscriptionMap to remove itself.
// This causes an internal noexcept function to throw since it is traversing a tree that is no longer in a valid state.
inline std::unordered_map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>, CELL::CELL_HASH> CELL::subscriptionMap{ };	// <Subject, Observers>
inline std::unordered_map<CELL::CELL_POSITION, std::shared_ptr<CELL>, CELL::CELL_HASH> CELL::cellMap{ };					// Stores all CELLs
inline std::mutex CELL::lkSubMap{ }, CELL::lkCellMap{ };

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
// Functions use lazy evaluation and support parallel evaluation.
class FUNCTION_CELL : public NUMERICAL_CELL {
public:
	void InitializeCell() override;
	void UpdateCell() override;		// Need to add recalculate logic here for when reference cells update

	struct ARGUMENT {
		virtual bool UpdateArgument() noexcept { return true; }				// Logic to update argument when dependent cells update. May be trivial.
		double Get() { return stillValid ? storedArgument : val.get(); }	// Lazy evaluation
	protected:
		std::future<double> val;
		double storedArgument{ };
		bool stillValid{ false };
		void SetValue(double) noexcept;
		void SetValue(std::exception) noexcept;
	};

	struct FUNCTION : public ARGUMENT {
		FUNCTION() = default;
		FUNCTION(std::vector<std::shared_ptr<ARGUMENT>>&&);
		std::vector<std::shared_ptr<ARGUMENT>> Arguments;
		//double (*funPTR) (vector<ARGUMENT>);			// Alternate to subclassing, just assign a function to this pointer at runtime.
		bool UpdateArgument() noexcept override;
		bool error{ false };
	};

	struct VALUE_ARGUMENT : public ARGUMENT {
		explicit VALUE_ARGUMENT(double);
		bool UpdateArgument() noexcept override;
	};

	struct REFERENCE_ARGUMENT : public ARGUMENT {
		REFERENCE_ARGUMENT(FUNCTION_CELL&, CELL_POSITION);
		~REFERENCE_ARGUMENT() { UnsubscribeFromCell(referencePosition, parentPosition); }
		CELL_POSITION referencePosition, parentPosition;
		bool UpdateArgument() noexcept override;
	};

protected:
	std::shared_ptr<ARGUMENT> func;

	std::shared_ptr<FUNCTION_CELL::ARGUMENT> ParseFunctionString(std::string&);
public:
	/*////////////////////////////////////////////////////////////
	// List of available operations overriding Function
	*/////////////////////////////////////////////////////////////
	struct SUM : public FUNCTION { SUM(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() noexcept final; };
	struct AVERAGE : public FUNCTION { AVERAGE(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() noexcept final; };
	struct PRODUCT : public FUNCTION { PRODUCT(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() noexcept final; };
	struct INVERSE : public FUNCTION { INVERSE(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() noexcept final; };
	struct RECIPROCAL : public FUNCTION { RECIPROCAL(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() noexcept final; };
	struct PI : public FUNCTION { PI(); bool UpdateArgument() noexcept final; };

	// Part of an alternative function mapping scheme
	// std::unordered_map<wstring, shared_ptr<FUNCTION_CELL::FUNCTION>> functionNameMap{ {wstring(L"SUM"), shared_ptr<FUNCTION_CELL::SUM>()}, {wstring(L"AVERAGE"), shared_ptr<FUNCTION_CELL::AVERAGE>()} };
};

#endif // !CELL_CLASS_H