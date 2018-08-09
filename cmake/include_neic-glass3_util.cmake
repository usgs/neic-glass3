# include_neic-glass3_util.cmake  - a CMake script that finds the util package,
# and sets up the includes
#
find_package(util CONFIG REQUIRED)
include_directories(${util_INCLUDE_DIRS})
include_directories(${spdlog_INCLUDE_DIRS}/..)
include_directories(${spdlog_INCLUDE_DIRS})
