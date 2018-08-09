# include_neic-glass3_process.cmake  - a CMake script that finds the process
# package, and sets up the includes
#
find_package(process CONFIG REQUIRED)
include_directories(${process_INCLUDE_DIRS})
