# Add source to this project's executable.
add_executable (Spreadsheet-Application WIN32 "Windows Infrastructure.cpp")
target_compile_definitions(Spreadsheet-Application
  PRIVATE "UNICODE;_UNICODE"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Spreadsheet-Application PROPERTY CXX_STANDARD 17)
endif()

target_sources(Spreadsheet-Application PRIVATE
	About.cpp
	SpreadsheetApplication.cpp
	SpreadsheetApplication.rc
	Cell.cpp
	Cell_Functions.cpp
	Table_Windows_OS.cpp
)
