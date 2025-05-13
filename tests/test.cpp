#include <catch2/catch_test_macros.hpp>
#include "Cell.hpp"
#include "Table.hpp"
#include <optional>

class TEST_TABLE : public TABLE_BASE {
public:
	void InitializeTable() override { };
	void PrintCellList() const { };
	void Redraw() const override { };
	void Undo() const override { };
	void Redo() const override { };
	CELL::CELL_PROXY CreateNewCell() const { return CELL::CELL_PROXY{ }; };
	CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const override { return CELL::CELL_PROXY{ nullptr }; };
	void ClearCell(const CELL::CELL_POSITION) const { };
	CELL::CELL_POSITION RequestCellPos() const { return CELL::CELL_POSITION{ }; };

	mutable std::optional<CELL::CELL_POSITION> lastUpdatedPosition_;
protected:
	// Unused functions
	// @todo, remove unused functions from base class interface
	// Derived interfaces can cast to the derived type to get additional operations
	void Resize() override { }
	void AddRow() override { }
	void AddColumn() override { }
	void RemoveRow() override { }
	void RemoveColumn() override { }
	unsigned int GetNumColumns() const override { return 0; }
	unsigned int GetNumRows() const override { return 0; }
	void FocusCell(const CELL::CELL_POSITION) const override { }
	void UnfocusCell(const CELL::CELL_POSITION) const override { }
	void FocusEntryBox() const override { }
	void UnfocusEntryBox(const CELL::CELL_POSITION) const override { }
	void FocusUp1(const CELL::CELL_POSITION) const override { }
	void FocusDown1(const CELL::CELL_POSITION) const override { }
	void FocusRight1(const CELL::CELL_POSITION) const override { }
	void FocusLeft1(const CELL::CELL_POSITION) const override { }
	void LockTargetCell(const CELL::CELL_POSITION) const override { }
	void ReleaseTargetCell() const override { }
	void UpdateCell(const CELL::CELL_POSITION position) const override { lastUpdatedPosition_ = position; };
	CELL::CELL_POSITION TargetCellGet() const override { return CELL::CELL_POSITION{ }; }
};

TEST_CASE("Invalid Position Returns No Cell") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto invalidPosition = CELL::CELL_POSITION{ 0, 0 };
	constexpr auto rawText = std::string_view{ "TEST" };

	auto cell = CELL::NewCell(&cellData, invalidPosition, std::string{ rawText });
	CHECK_FALSE(bool{ cell });
}

TEST_CASE("Empty Text Returns No Cell") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto emptyText = std::string_view{ "" };
	
	auto cell = CELL::NewCell(&cellData, position, std::string{ emptyText });
	CHECK_FALSE(bool{ cell });
}

TEST_CASE("Factory Creates Cell with Matching Position & Raw Text") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto rawText = std::string_view{ "'abc" };

	auto cell = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ cell });
	CHECK(cell->GetPosition() == position);
	CHECK(cell->GetRawContent() == rawText.data());
}

TEST_CASE("Factory Stores Cell In Cell Data For Later Reference") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto rawText = std::string_view{ "abc" };

	auto cell = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ cell });
	auto sameUnderlyingCell = cellData.GetCellProxy(position);
	REQUIRE(bool{ sameUnderlyingCell });
	CHECK(cell == sameUnderlyingCell);
}

TEST_CASE("Cells Are Idempotent") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto rawText = std::string_view{ "abc" };

	auto cell = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ cell });
	auto madeFromSameInputData = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ madeFromSameInputData });
	CHECK(cell == madeFromSameInputData);
}

TEST_CASE("Factory Calls Update on Table for Cell Position") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto rawText = std::string_view{ "abc" };

	auto cell = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ cell });
	auto casted = static_cast<TEST_TABLE*>(table.get());
	CHECK(casted->lastUpdatedPosition_ == position);
}

TEST_CASE("Text Cell Displays Matching Text") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto unambiguousText = std::string_view{ "abc" };

	auto cell = CELL::NewCell(&cellData, position, std::string{ unambiguousText });
	REQUIRE(bool{ cell });
	CHECK(cell->GetOutput() == unambiguousText.data());
}

TEST_CASE("Preceeding Apostrophe Forces Interpretation As Text") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	constexpr auto numbersAsText = std::string_view{ "123" };

	auto numberTextCell = CELL::NewCell(&cellData, position, std::string{ '\'' } + numbersAsText.data());
	REQUIRE(bool{ numberTextCell });
	CHECK(numberTextCell->GetOutput() == numbersAsText.data());

	constexpr auto referenceAsText = std::string_view{ "&R1C1" };

	auto referenceTextCell = CELL::NewCell(&cellData, position, std::string{ '\'' } + referenceAsText.data());
	REQUIRE(bool{ referenceTextCell });
	CHECK(referenceTextCell->GetOutput() == referenceAsText.data());

	constexpr auto functionAsText = std::string_view{ "= SUM(1, 2, 3)" };

	auto functionTextCell = CELL::NewCell(&cellData, position, std::string{ '\'' } + functionAsText.data());
	REQUIRE(bool{ functionTextCell });
	CHECK(functionTextCell->GetOutput() == functionAsText.data());
}
