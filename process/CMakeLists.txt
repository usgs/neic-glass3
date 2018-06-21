# neic-glass3 process library CMake configuration file.
#
# This file contains the CMake configuration file that builds the associator
# processing library for neic-glass3. This configuration follows the "CMake
# Superbuilds and Git Submodules" scheme to organize and orchestrate the build 
# process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (process VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

#----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
find_package(SuperEasyJSON CONFIG REQUIRED)
include_directories(${SuperEasyJSON_INCLUDE_DIRS})

# log
find_package(log CONFIG REQUIRED)
include_directories(${log_INCLUDE_DIRS})
include_directories(${spdlog_INCLUDE_DIRS}/..)
include_directories(${spdlog_INCLUDE_DIRS})

# util
find_package(util CONFIG REQUIRED)
include_directories(${util_INCLUDE_DIRS})

# glasscore
find_package(glasscore CONFIG REQUIRED)
include_directories(${glasscore_INCLUDE_DIRS}/glasslib/include)
include_directories(${glasscore_INCLUDE_DIRS}/glassutil/include)
include_directories(${glasscore_INCLUDE_DIRS}/traveltime/include)

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories("${PROJECT_BINARY_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/include)

# ----- SET SOURCE FILES ----- #
file(GLOB SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp)

# ----- SET HEADER FILES ----- #
file(GLOB HDRS ${PROJECT_SOURCE_DIR}/include/*.h)

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
# no tests currently

# ----- INSTALL LIBRARY ----- #
include(${CMAKE_DIR}/install_lib.cmake)
