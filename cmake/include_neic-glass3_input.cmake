# include_neic-glass3_input.cmake  - a CMake script that finds the input package,
# and sets up the includes
#
find_package(input CONFIG REQUIRED)
include_directories(${input_INCLUDE_DIRS})
