# SuperEasyJSON library CMake configuration file.
#
# This file contains the CMake configuration file that builds the SuperEasyJSON
# library. This configuration follows the "CMake Superbuilds and
# Git Submodules" scheme to organize and orchestrate the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../cmake/)

# ----- PROJECT ----- #
project (SuperEasyJSON VERSION 2.2.0)

#----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories(${PROJECT_SOURCE_DIR})

# ----- SET SOURCE FILES ----- #
set (SRCS ${SRCS} ${PROJECT_SOURCE_DIR}/json.cpp
)

# ----- SET HEADER FILES ----- #
set (HDRS ${HDRS} ${PROJECT_SOURCE_DIR}/json.h
)

# ----- BUILD LIBRARY ----- #
include(${CMAKE_DIR}/build_lib.cmake)

# ----- INSTALL LIBRARY ----- #
include(${CMAKE_DIR}/install_lib.cmake)
