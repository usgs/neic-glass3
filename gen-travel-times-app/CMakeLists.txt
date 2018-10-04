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
include(${CMAKE_DIR}/include_SuperEasyJSON.cmake)

# ----- GLASS3 LIBRARIES ----- #
# util
include(${CMAKE_DIR}/include_neic-glass3_util.cmake)

# glasscore
include(${CMAKE_DIR}/include_neic-glass3_glasscore.cmake)

# ----- SET SOURCE FILES ----- #
set(SRCS "${PROJECT_SOURCE_DIR}/gen-travel-times-app.cpp")

# ----- SET HEADER FILES ----- #
# no headers

# ----- BUILD EXECUTABLE ----- #
# WARNING: linking order of libraries matters for G++
set(EXE_LIBRARIES ${SuperEasyJSON_LIBRARIES} ${glasscore_LIBRARIES} ${util_LIBRARIES})
include(${CMAKE_DIR}/build_exe.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
# no tests currently

# ----- INSTALL EXECUTABLE ----- #
include(${CMAKE_DIR}/install_exe.cmake)
