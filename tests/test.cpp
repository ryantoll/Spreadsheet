#include <catch2/catch_test_macros.hpp>
#undef _WINDOWS
#include "Cell.hpp"
#include "Table.hpp"

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
protected:
	mutable CELL::CELL_DATA cellData;
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> undoStack{ };
	mutable std::vector<std::pair<CELL::CELL_PROXY, CELL::CELL_PROXY>> redoStack{ };

	// Unused functions
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
	void UpdateCell(const CELL::CELL_POSITION) const override { };
	CELL::CELL_POSITION TargetCellGet() const override { return CELL::CELL_POSITION{ }; }
};

TEST_CASE("Invalid Position Returns No Cell") {
	table = std::make_unique<TEST_TABLE>();

	auto invalidPosition = CELL::CELL_POSITION{ 0, 0 };
	constexpr auto rawText = std::string_view{ "TEST" };
	auto cellData = CELL::CELL_DATA{ };
	auto cell = CELL::NewCell(&cellData, invalidPosition, std::string{ rawText });
	CHECK_FALSE(bool{ cell });
}


TEST_CASE("Text Cell Displays Matching Text") {
	table = std::make_unique<TEST_TABLE>();
	auto cellData = CELL::CELL_DATA{ };
	auto position = CELL::CELL_POSITION{ 1, 1 };
	auto rawText = "TEST";

	auto cell = CELL::NewCell(&cellData, position, std::string{ rawText });
	REQUIRE(bool{ cell });
	CHECK(cell->GetPosition() == position);
	CHECK(cell->GetRawContent() == rawText);
	CHECK(cell->GetOutput() == rawText);
}

