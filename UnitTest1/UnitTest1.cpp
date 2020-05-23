#include "pch.h"
#include "CppUnitTest.h"
#define _WINDOWS	// Need to define precompiler flag here so that it properly propigates through code compilation
//#define _CONSOLE	// Otherwise, it could entirely miss relevant segments, so no implementation is defined
#include "stdafx.h"
#include "__Table.h"

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		// Empty text returns empty cell proxy
		TEST_METHOD(Empty_Cell) {
			table = std::make_unique<WINDOWS_TABLE>();
			auto cell = table->CreateNewCell({ 1, 1 }, "");
			Assert::IsFalse(bool{ cell });
		}

		// Purposeful error
		TEST_METHOD(Function_Error) {
			table = std::make_unique<WINDOWS_TABLE>();
			auto cell = table->CreateNewCell({ 1, 1 }, "=&R5C5");
			auto text = cell->GetOutput();
			auto expectedText = "!ERROR!"s;
			Assert::AreEqual(expectedText, text);
		}

		// Test that functions update properly when underlying values change
		TEST_METHOD(Function_Updates) {
			table = std::make_unique<WINDOWS_TABLE>();

			// Insert test cells
			table->CreateNewCell({ 1, 1 }, "2");
			table->CreateNewCell({ 2, 1 }, "4");
			table->CreateNewCell({ 3, 1 }, "9");
			auto cell = table->CreateNewCell({ 5, 1 }, "=SUM( &R1C1, &R1C2, &R1C3 )");

			table->CreateNewCell({ 1, 1 }, "7");		// Change a cell
			auto result = stoi(cell->GetOutput());

			Assert::AreEqual(20, result);	// 7 + 4 + 9 = 20
		}
	};
}
