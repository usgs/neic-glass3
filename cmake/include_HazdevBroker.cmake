# include_HazdevBroker.cmake  - a CMake script that finds the HazdevBroker
# package and sets up the includes
#
# HazDevBroker
find_package(HazdevBroker CONFIG REQUIRED)
include_directories(${HazdevBroker_INCLUDE_DIRS})
