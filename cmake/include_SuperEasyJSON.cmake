# include_SuperEasyJSON.cmake  - a CMake script that finds the SuperEasyJSON
# package, and sets up the includes
#
find_package(SuperEasyJSON CONFIG REQUIRED)
include_directories(${SuperEasyJSON_INCLUDE_DIRS})
