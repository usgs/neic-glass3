# neic-glass3 gen-travel-times-app application CMake configuration file.
#
# This file contains the CMake configuration file that builds the
# gen-travel-times-app application for neic-glass3. This configuration follows
# the "CMake Superbuilds and Git Submodules" scheme to organize and orchestrate
# the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (gen-travel-times-app VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

#----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
find_package(SuperEasyJSON REQUIRED)
include_directories(${SuperEasyJSON_INCLUDE_DIRS})

# glasscore
find_package(glasscore CONFIG REQUIRED)
include_directories(${glasscore_INCLUDE_DIRS})

# log
find_package(log CONFIG REQUIRED)
include_directories(${log_INCLUDE_DIRS})
include_directories(${spdlog_INCLUDE_DIRS}/..)
include_directories(${spdlog_INCLUDE_DIRS})

# config
find_package(config CONFIG REQUIRED)
include_directories(${config_INCLUDE_DIRS})

# util
find_package(util CONFIG REQUIRED)
include_directories(${util_INCLUDE_DIRS})

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories ("${PROJECT_BINARY_DIR}")

# ----- SET SOURCE FILES ----- #
set(SRCS "${PROJECT_SOURCE_DIR}/gen-travel-times-app.cpp")

# ----- SET HEADER FILES ----- #
# no headers

# ----- BUILD EXECUTABLE ----- #
# NOTE: Order libraries are linked matters for G++
set(EXE_LIBRARIES ${SuperEasyJSON_LIBRARIES} ${glasscore_LIBRARIES} ${log_LIBRARIES} ${config_LIBRARIES} ${util_LIBRARIES})
include(${CMAKE_DIR}/build_exe.cmake)

# ----- GENERATE DOCUMENTATION ----- #
set(DOC_DIRS "${PROJECT_SOURCE_DIR}")
include(${CMAKE_DIR}/documentation.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
# no tests currently

# ----- INSTALL EXECUTABLE ----- #
include(${CMAKE_DIR}/install_exe.cmake)
