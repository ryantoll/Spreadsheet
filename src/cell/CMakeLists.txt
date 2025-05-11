# Add source to this project's executable.
add_library(cell Cell.cpp Cell_Functions.cpp)
target_include_directories(cell PUBLIC .)
