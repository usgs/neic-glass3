# neic-glass3 input library CMake configuration file.
#
# This file contains the CMake configuration file that builds the input
# library for neic-glass3. This configuration follows the "CMake Superbuilds and
# Git Submodules" scheme to organize and orchestrate the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (input VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

#----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
include(${CMAKE_DIR}/include_SuperEasyJSON.cmake)

# detection-formats
include(${CMAKE_DIR}/include_DetectionFormats.cmake)

# ----- GLASS3 LIBRARIES ----- #
# util
include(${CMAKE_DIR}/include_neic-glass3_util.cmake)

# parse
include(${CMAKE_DIR}/include_neic-glass3_parse.cmake)

# ----- SET SOURCE FILES ----- #
file(GLOB SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp")

# ----- SET HEADER FILES ----- #
file(GLOB HDRS "${PROJECT_SOURCE_DIR}/include/*.h")

# ----- BUILD LIBRARY ----- #
include(${CMAKE_DIR}/build_lib.cmake)

# ----- GENERATE DOCUMENTATION ----- #
set(DOC_DIRS "${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src")
include(${CMAKE_DIR}/documentation.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
file(GLOB TESTS ${PROJECT_SOURCE_DIR}/tests/*.cpp)
# WARNING: linking order of libraries matters for G++
set(TEST_LIBRARIES ${parse_LIBRARIES} ${DetectionFormats_LIBRARIES} ${util_LIBRARIES} ${SuperEasyJSON_LIBRARIES})
include(${CMAKE_DIR}/test.cmake)

# ----- INSTALL LIBRARY ----- #
include(${CMAKE_DIR}/install_lib.cmake)
