# include_neic-glass3_parse.cmake  - a CMake script that finds the parse package,
# and sets up the includes
#
find_package(parse CONFIG REQUIRED)
include_directories(${parse_INCLUDE_DIRS})
