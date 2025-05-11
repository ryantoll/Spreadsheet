# Add source to this project's executable.
add_executable(Spreadsheet-Console-UI "Table_Console_Application.cpp")
target_compile_definitions(Spreadsheet-Console-UI PUBLIC "_CONSOLE")
target_link_libraries(Spreadsheet-Console-UI PRIVATE cell)