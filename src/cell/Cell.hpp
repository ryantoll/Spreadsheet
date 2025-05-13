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
// However, hashing may take long enough that O(1) > O( log(n) ) for small values of n.
// Also, a 2-D array could be used to get O(1) speed plus cache localization at the cost of many empty slots.
// CELL_POSITION defines it's own operator< and operator== for use in map sorting as well as a hash function.
// The choice of column sorting preempting row sorting is arbitrary. Either way is fine so long as it is consistent.
*////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CELL_CLASS_HPP
#define CELL_CLASS_HPP

#include <memory>

#include <future>
#include <set>
#include <string>

constexpr auto MaxRow_{ UINT16_MAX };
constexpr auto MaxColumn_{ UINT16_MAX };

// Base class for all cells.
// It stores the raw input string, a display value of that string, and returns the protected display value.
class CELL {
public:
	// Position of cell: Column, Row
	struct CELL_POSITION {
		unsigned int column{ 0 };
		unsigned int row{ 0 };
	};

	// Hash function of CELL_POSITION
	// It is simply a bitwise concatination of the column and row bits.
	struct CELL_HASH {
		std::size_t operator() (CELL::CELL_POSITION const& pos) const {
			auto x = unsigned long{ 0 };
			x = x | pos.column;
			x = x << 16;
			x = x | pos.row;
			return x;
		}
	};

	// Any changes to a CELL should trigger a notification
	// Utilizing a "Proxy" pattern ensures that a notification is sent whenever a change is made
	// Users of CELL have only indirect access to CELLs since they should not be responsible for sending notifications.
	class CELL_PROXY {
		friend class CELL;
		std::shared_ptr<CELL> cell;
		auto operator*() const { return *cell; }
	public:
		CELL_PROXY() = default;
		explicit CELL_PROXY(std::shared_ptr<CELL> target) : cell{ std::move(target) } { }
		~CELL_PROXY() = default;

		// Custom copy & move update cell. Move constructors swallow errors to allow typical noexcept behavior
		CELL_PROXY& operator=(const CELL_PROXY& target) { cell = target.cell; if (cell) { cell->UpdateCell(); } return *this; }
		CELL_PROXY(const CELL_PROXY& target) : cell{target.cell} { cell = target.cell; if (cell) { cell->UpdateCell(); } }

		CELL_PROXY& operator=(CELL_PROXY&& target) {
			cell = std::move(target.cell);
			try { if (cell) { cell->UpdateCell(); } }
			catch (...) { /*swallow errors*/ }
			return *this;
		}

		CELL_PROXY(CELL_PROXY&& target) noexcept : cell{ std::move(target.cell) } {
			try { if (cell) { cell->UpdateCell(); } }
			catch (...) { /*swallow errors*/ }
		}

		// Can share in or assume ownership of cell pointer
		CELL_PROXY& operator=(const std::shared_ptr<CELL>& target) { cell = target; if (cell) { cell->UpdateCell(); } return *this; }

		CELL_PROXY& operator=(std::shared_ptr<CELL>&& target) noexcept {
			cell = std::move(target);
			try { if (cell) { cell->UpdateCell(); } }
			catch (...) { /*swallow errors*/ }
			return *this;
		}

		auto operator->() const { return cell; }
		explicit operator bool() const { return bool{ cell }; }

		friend bool operator== (const CELL::CELL_PROXY& lhs, const CELL::CELL_PROXY& rhs) { return lhs.cell.get() == rhs.cell.get(); }

		friend bool operator!= (const CELL::CELL_PROXY& lhs, const CELL::CELL_PROXY& rhs) { return !(lhs == rhs); }
	};

	// Allows client to store all CELL data and subscriptions, allowing any number of sets as needed.
	// An "Observer" pattern is used to notify relevant cells when changes occur.
	// Changes may cascade, so notifications need to handle that effectively.
	// Stores a set of observers for each subject.
	// Automatically synchronizes data access for threading (even for non-const functions).
	// While not needed in this use case, it is still valuable for demonstration purposes and may be used later.
	// Uses a double layer of encapsulation to provide different levels of access to different clients.
	// Clients of CELL class get a largely opaque data structure that only provides indirect access to cells through a proxy.
	// CELL needs some extra privilages to manage cell data, but need to be constrianed to the threadsafe interface.
	class CELL_DATA {
		class INNER_CELL_DATA {
			std::unordered_map<CELL::CELL_POSITION, std::set<CELL::CELL_POSITION>, CELL_HASH> subscriptionMap;	// <Subject, (set of) Observers>
			std::unordered_map<CELL::CELL_POSITION, std::shared_ptr<CELL>, CELL_HASH> cellMap;					// Cell data
			mutable std::mutex lkSubMap, lkCellMap;
			friend class CELL_DATA;
		};

		INNER_CELL_DATA data;
		std::shared_ptr<CELL> GetCell(const CELL::CELL_POSITION) const;
		void NotifyAll(const CELL_POSITION) const;
		void AssignCell(const std::shared_ptr<CELL>);
		void EraseCell(const CELL_POSITION);
		void SubscribeToCell(const CELL_POSITION, const CELL_POSITION);
		void UnsubscribeFromCell(const CELL_POSITION, const CELL_POSITION);
	public:
		CELL_PROXY GetCellProxy(const CELL::CELL_POSITION);
		friend class CELL;
		friend struct REFERENCE_ARGUMENT;
	};

	// "Factory" function to create new cells
	// This was previously written as a class, but has devolved over time as it is only a single function in practice
	// A function parallels the "Singleton" pattern, but implies that users cannot extend it through inheritance
	static CELL_PROXY NewCell(CELL_DATA*, const CELL_POSITION, const std::string&);
	static void RecreateCell(CELL_DATA*, const CELL_PROXY&, const CELL_POSITION);

protected:
	CELL() { }		// Hide constructor to force usage of factory function
private:
	CELL(const CELL_PROXY cell) { *this = *cell; parentContainer->NotifyAll(position); }		// Create cell from cell proxy and notify of change
public:
	virtual ~CELL() { }

private:
	std::string rawContent;
protected:
	bool error{ false };
	std::string displayValue;
	CELL_POSITION position;
	CELL_DATA* parentContainer{ nullptr };

	void SubscribeToCell(const CELL_POSITION) const;
	void UnsubscribeFromCell(const CELL_POSITION) const;
public:
	virtual std::string GetOutput() const { return error ? "!ERROR!" : displayValue; }
	virtual std::string GetRawContent() const { return rawContent; }
	virtual void InitializeCell() { displayValue = rawContent; }
	virtual void UpdateCell();						// Tell a CELL to update its state.
	CELL_POSITION GetPosition() const { return position; }
};

inline bool operator< (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) {
	if (lhs.column < rhs.column) { return true; }
	else if (lhs.column == rhs.column && lhs.row < rhs.row) { return true; }
	else { return false; }
}

inline bool operator== (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) { return lhs.column == rhs.column && lhs.row == rhs.row ? true : false; }

inline bool operator!= (const CELL::CELL_POSITION& lhs, const CELL::CELL_POSITION& rhs) { return !(lhs == rhs); }

// A cell that is simply raw text merely outputs its text.
class TEXT_CELL : public CELL {
public:
	virtual ~TEXT_CELL() {}
	std::string GetOutput() const override { return displayValue; }		// Presumably this will never be in an error state.
	void InitializeCell() override;
};

// A cell that refers to another cell by referring to it's position.
class REFERENCE_CELL : public CELL {
public:
	virtual ~REFERENCE_CELL() { UnsubscribeFromCell(referencePosition); }
	std::string GetOutput() const override {
		auto out = std::string{ };
		auto cell = parentContainer->GetCellProxy(referencePosition);
		if (!cell || cell->GetPosition() == position) { out = "!REF!"; }	// Dangling reference & reference to self both cause a reference error.
		else { out = cell->GetOutput(); }
		return error ? "!ERROR!" : out;
	}
	void InitializeCell() override;
protected:
	CELL_POSITION referencePosition;
};

// Parese string into Row & Column positions of reference cell
// Parsing allows for either ordering and is not case-sensitive
CELL::CELL_POSITION ReferenceStringToCellPosition(const std::string& refString);

// A base class for all cells that contains numbers.
class NUMERICAL_CELL : public CELL {
protected:
	double storedValue{ 0 };
	//DISPLAY_PARAMETERS parameters;		// Add criteria for textual representation of value. (Ex. 1 vs. 1.0000 vs. $1.00, etc.)
public:
	virtual ~NUMERICAL_CELL() {}
	std::string GetOutput() const override { return error ? "!ERROR!" : std::to_string(storedValue); }
	void InitializeCell() override;
};

struct ARGUMENT;
// A cell that contains one or more FUNCTION(s).
class FUNCTION_CELL : public NUMERICAL_CELL {
public:
	void InitializeCell() override;
	void UpdateCell() override;		// Need to add recalculate logic here for when reference cells update
protected:
	std::shared_ptr<ARGUMENT> m_Func;
	std::shared_ptr<ARGUMENT> ParseFunctionString(std::string&);
};

// ARGUMENT serves as the argument for FUNCTIONs, which are in turn ARGUMENTs themselves.
// It contains a future to get its value from an async call, stores the value, and tracks changes in underlying arguments.
struct ARGUMENT {
	virtual bool UpdateArgument() { return true; }				// Logic to update argument when dependent cells update. May be trivial.
	double Get() { return stillValid ? storedArgument : val.get(); }	// Lazy evaluation
protected:
	std::future<double> val;
	double storedArgument{ };
	bool stillValid{ false };
	void SetValue(double);
	void SetValue(std::exception);
};

// FUNCTION will utilize the "Strategy" pattern to support numerous function types by specializing the base FUNCTION type for each function used.
// FUNCTION_CELL will utilize the "Composite" pattern to treat singular and aggregate FUNCTIONS uniformly.
// Each FUNCTION both takes ARGUMENTs and is itself an ARGUMENT, allowing for recursive composition.
// Alternatively, FUNCTION could simply store a function pointer that stores the appropriate function at runtime based upon function name lookup.
// Each function must have the same call signature to fit in the same pointer object (taking a vector of ARGUMENTS and storing a double).
// The sub-classing option is used here to comport with the broader goal of demonstrating established Object-oriented design patterns.
// This alternate strategy is shown in comments for completeness.
// Functions use lazy evaluation and support parallel evaluation and avoid duplicate work.
struct FUNCTION : public ARGUMENT {
	FUNCTION() = default;
	FUNCTION(std::vector<std::shared_ptr<ARGUMENT>>&&);
	std::vector<std::shared_ptr<ARGUMENT>> Arguments;
	bool error{ false };
	//double (*funPTR) (vector<ARGUMENT>);			// Alternate to subclassing, just assign a function to this pointer at runtime.
	bool UpdateArgument() override;
};

struct VALUE_ARGUMENT : public ARGUMENT {
	explicit VALUE_ARGUMENT(double);
	bool UpdateArgument() override;
};

struct REFERENCE_ARGUMENT : public ARGUMENT {
	REFERENCE_ARGUMENT(CELL::CELL_DATA*, FUNCTION_CELL&, CELL::CELL_POSITION);
	~REFERENCE_ARGUMENT();
	CELL::CELL_DATA* parentContainer;
	CELL::CELL_POSITION referencePosition, parentPosition;
	bool UpdateArgument() override;
};

/*////////////////////////////////////////////////////////////
// List of available operations overriding Function
*/////////////////////////////////////////////////////////////

// I'm not sure how best to implement this mapping of text to function objects.
// Each one must get a newly created object, lest they share the same arguments.
std::shared_ptr<FUNCTION> MatchNameToFunction(const std::string& inputText, std::vector<std::shared_ptr<ARGUMENT>>&& args);

struct SUM : public FUNCTION { SUM(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() final; };
struct AVERAGE : public FUNCTION { AVERAGE(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() final; };
struct PRODUCT : public FUNCTION { PRODUCT(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() final; };
struct INVERSE : public FUNCTION { INVERSE(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() final; };
struct RECIPROCAL : public FUNCTION { RECIPROCAL(std::vector<std::shared_ptr<ARGUMENT>>&& args); bool UpdateArgument() final; };
struct PI : public FUNCTION { PI(); bool UpdateArgument() final; };

// Part of an alternative function mapping scheme
// std::unordered_map<wstring, shared_ptr<FUNCTION_CELL::FUNCTION>> functionNameMap{ {wstring(L"SUM"), shared_ptr<FUNCTION_CELL::SUM>()}, {wstring(L"AVERAGE"), shared_ptr<FUNCTION_CELL::AVERAGE>()} };

#endif // !CELL_CLASS_HPP