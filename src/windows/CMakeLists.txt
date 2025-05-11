# Add source to this project's executable.
add_executable (Spreadsheet-Windows-GUI WIN32 "Windows Infrastructure.cpp")
target_compile_definitions(Spreadsheet-Windows-GUI PUBLIC "UNICODE;_UNICODE;WIN32_LEAN_AND_MEAN")
target_link_libraries(Spreadsheet-Windows-GUI PRIVATE cell)
target_sources(Spreadsheet-Windows-GUI PRIVATE
	About.cpp
	SpreadsheetApplication.cpp
	SpreadsheetApplication.rc
	Table_Windows_OS.cpp
)
