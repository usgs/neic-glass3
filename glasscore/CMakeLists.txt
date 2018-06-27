# neic-glass3 glasscore library CMake configuration file.
#
# This file contains the CMake configuration file that builds the core glass
# library for neic-glass3. This configuration follows the "CMake Superbuilds and
# Git Submodules" scheme to organize and orchestrate the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (glasscore VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

# ----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
find_package(SuperEasyJSON CONFIG REQUIRED)
include_directories(${SuperEasyJSON_INCLUDE_DIRS})

# uuid
include(${CMAKE_DIR}/uuid.cmake)

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories("${PROJECT_BINARY_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/glassutil/include)
include_directories(${PROJECT_SOURCE_DIR}/traveltime/include)
include_directories(${PROJECT_SOURCE_DIR}/glasslib/include)

# ----- SET SOURCE FILES ----- #
file(GLOB SRCS "${PROJECT_SOURCE_DIR}/glassutil/src/*.cpp"
               "${PROJECT_SOURCE_DIR}/traveltime/src/*.cpp"
               "${PROJECT_SOURCE_DIR}/glasslib/src/*.cpp")

# ----- SET HEADER FILES ----- #
file(GLOB HDRS "${PROJECT_SOURCE_DIR}/glassutil/include/*.h"
               "${PROJECT_SOURCE_DIR}/traveltime/include/*.h"
               "${PROJECT_SOURCE_DIR}/glasslib/include/*.h")

# ----- BUILD LIBRARY ----- #
include(${CMAKE_DIR}/build_lib.cmake)

# ----- GENERATE DOCUMENTATION ----- #
set(DOC_DIRS "${PROJECT_SOURCE_DIR}/glassutil ${PROJECT_SOURCE_DIR}/traveltime ${PROJECT_SOURCE_DIR}/glasslib")
include(${CMAKE_DIR}/documentation.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
file(GLOB TESTS ${PROJECT_SOURCE_DIR}/tests/*.cpp)
# NOTE: Order libraries are linked matters for G++
set(TEST_LIBRARIES ${SuperEasyJSON_LIBRARIES})
if (UNIX AND NOT APPLE)
    set(TEST_LIBRARIES ${TEST_LIBRARIES} ${LIBUUID_LIBRARY})
endif (UNIX AND NOT APPLE)
include(${CMAKE_DIR}/test.cmake)

# ----- INSTALL LIBRARY ----- #
include(${CMAKE_DIR}/install_lib.cmake)
