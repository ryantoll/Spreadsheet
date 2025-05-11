# Add source to this project's executable.
add_executable (Spreadsheet-Windows-GUI WIN32 "Windows Infrastructure.cpp")
target_compile_definitions(Spreadsheet-Windows-GUI
  PRIVATE "UNICODE;_UNICODE;WIN32_LEAN_AND_MEAN"
)
target_sources(Spreadsheet-Windows-GUI PRIVATE
	About.cpp
	SpreadsheetApplication.cpp
	SpreadsheetApplication.rc
	Cell.cpp
	Cell_Functions.cpp
	Table_Windows_OS.cpp
)

add_executable (Spreadsheet-Console-UI "Table_Console_Application.cpp")
target_compile_definitions(Spreadsheet-Console-UI PRIVATE "UNICODE;_UNICODE;_CONSOLE")

target_sources(Spreadsheet-Console-UI PRIVATE
	Cell.cpp
	Cell_Functions.cpp
)
