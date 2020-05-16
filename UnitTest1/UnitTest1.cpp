#include "pch.h"
#include "CppUnitTest.h"
#define _WINDOWS	// Need to define precompiler flag here so that it properly propigates through code compilation
//#define _CONSOLE	// Otherwise, it could entirely miss relevant segments, so no implementation is defined
#include "stdafx.h"
#include "__Table.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		
		// Test that functions update properly when underlying values change
		TEST_METHOD(Function_Updates)
		{
			table = std::make_unique<WINDOWS_TABLE>();
			//table = std::make_unique<CONSOLE_TABLE>();

			// Insert test cells
			CELL::cell_factory.NewCell({ 1, 1 }, "2");
			CELL::cell_factory.NewCell({ 1, 2 }, "4");
			CELL::cell_factory.NewCell({ 1, 3 }, "9");
			auto cell = CELL::cell_factory.NewCell({ 1, 5 }, "=SUM( &R1C1, &R1C2, &R1C3 )");

			CELL::cell_factory.NewCell({ 1, 1 }, "7");		// Change a cell
			auto result = stoi( cell->DisplayOutput());

			Assert::AreEqual(20, result);	// 7 + 4 + 9 = 20
		}
	};
}
