# include_neic-glass3_glasscore.cmake  - a CMake script that finds the glasscore
# package, and sets up the includes
#
find_package(glasscore CONFIG REQUIRED)
include_directories(${glasscore_INCLUDE_DIRS})
