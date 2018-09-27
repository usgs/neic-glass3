# neic-glass3 glass-app application CMake configuration file.
#
# This file contains the CMake configuration file that builds the
# glass-app application for neic-glass3. This configuration follows
# the "CMake Superbuilds and Git Submodules" scheme to organize and orchestrate
# the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (glass-app VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

#----- BASE FUNCTIONS ----- #
include(${CMAKE_DIR}/base.cmake)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
include(${CMAKE_DIR}/include_SuperEasyJSON.cmake)

# detection-formats
include(${CMAKE_DIR}/include_DetectionFormats.cmake)

# uuid
include(${CMAKE_DIR}/uuid.cmake)

# ----- GLASS3 LIBRARIES ----- #
# util
include(${CMAKE_DIR}/include_neic-glass3_util.cmake)

# parse
include(${CMAKE_DIR}/include_neic-glass3_parse.cmake)

# glasscore
include(${CMAKE_DIR}/include_neic-glass3_glasscore.cmake)

# process
include(${CMAKE_DIR}/include_neic-glass3_process.cmake)

# output
include(${CMAKE_DIR}/include_neic-glass3_output.cmake)

# input
include(${CMAKE_DIR}/include_neic-glass3_input.cmake)

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories(${PROJECT_SOURCE_DIR}/fileInput)
include_directories(${PROJECT_SOURCE_DIR}/fileOutput)

# ----- SET SOURCE FILES ----- #
file(GLOB SRCS "${PROJECT_SOURCE_DIR}/fileInput/*.cpp"
               "${PROJECT_SOURCE_DIR}/fileOutput/*.cpp"
               "${PROJECT_SOURCE_DIR}/*.cpp")

# ----- SET HEADER FILES ----- #
file(GLOB HDRS "${PROJECT_SOURCE_DIR}/fileInput/*.h"
               "${PROJECT_SOURCE_DIR}/fileOutput/*.h")

# ----- (testing) LIBRARY ----- #
# testingLib
add_library (testingLib STATIC ${SRCS} ${HDRS})

# ----- BUILD EXECUTABLE ----- #
# WARNING: linking order of libraries matters for G++
set(EXE_LIBRARIES ${input_LIBRARIES} ${output_LIBRARIES} ${process_LIBRARIES} ${parse_LIBRARIES} ${DetectionFormats_LIBRARIES} ${glasscore_LIBRARIES} ${util_LIBRARIES} ${log_LIBRARIES} ${SuperEasyJSON_LIBRARIES})
include(${CMAKE_DIR}/build_exe.cmake)

# ----- GENERATE DOCUMENTATION ----- #
set(DOC_DIRS "${PROJECT_SOURCE_DIR}/file_input ${PROJECT_SOURCE_DIR}/file_output ${PROJECT_SOURCE_DIR}/glass-app.cpp")
include(${CMAKE_DIR}/documentation.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
file(GLOB TESTS ${PROJECT_SOURCE_DIR}/tests/*.cpp)
# Just use the exe libraries
set(TEST_LIBRARIES testingLib ${EXE_LIBRARIES})
include(${CMAKE_DIR}/test.cmake)

# ----- INSTALL EXECUTABLE ----- #
include(${CMAKE_DIR}/install_exe.cmake)
