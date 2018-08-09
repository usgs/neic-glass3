# include_neic-glass3_output.cmake  - a CMake script that finds the output
# package, and sets up the includes
#
find_package(output CONFIG REQUIRED)
include_directories(${output_INCLUDE_DIRS})
