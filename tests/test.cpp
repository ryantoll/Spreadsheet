#include <catch2/catch_test_macros.hpp>
#undef _WINDOWS
#include "Cell.hpp"
#include "Table.hpp"

/// @todo create test table to store cells
/// may need to split out dependency in some way

TEST_CASE("Invalid Position Returns No Cell") {
	CHECK(true);

	//auto invalidPosition = CELL::CELL_POSITION{ 1, 1 };
	//constexpr auto rawText = std::string_view{ "TEST" };
	//auto cellData = CELL::CELL_DATA{};
	//auto cell = CELL::NewCell(&cellData, CELL::CELL_POSITION{ 1, 1 }, std::string{ rawText });
	//REQUIRE_FALSE(bool{ cell });

	//auto cell = table->CreateNewCell(CELL::CELL_POSITION{ 1, 1 }, std::string{ rawText });
	//REQUIRE_FALSE(bool{ cell });
	//REQUIRE(bool{ cell });
}


TEST_CASE("Text Cell Displays Text") {
	CHECK(true);
//	auto position = CELL::CELL_POSITION{ 1, 1 };
//	constexpr auto rawText = std::string_view{ "TEST" };
//
//	auto cell = table->CreateNewCell(CELL::CELL_POSITION{ 1, 1 }, std::string{ rawText });
//	REQUIRE(cell->GetPosition() == position);
//	REQUIRE(cell->GetRawContent() == rawText);
//	REQUIRE(cell->GetOutput() == rawText);
}

