# neic-glass3 glass-broker-app application CMake configuration file.
#
# This file contains the CMake configuration file that builds the
# glass-broker-app application for neic-glass3. This configuration follows
# the "CMake Superbuilds and Git Submodules" scheme to organize and orchestrate
# the build process.

cmake_minimum_required (VERSION 3.4)
set(CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake/)

# ----- PROJECT VERSION ----- #
include(${CMAKE_DIR}/version.cmake)

# ----- PROJECT ----- #
project (glass-broker-app VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH} )

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

# detection-formats
find_package(DetectionFormats CONFIG REQUIRED)
include_directories(${DetectionFormats_INCLUDE_DIRS})
include_directories(${RapidJSON_INCLUDE_DIRS})

# parse
find_package(parse CONFIG REQUIRED)
include_directories(${parse_INCLUDE_DIRS})

# process
find_package(process CONFIG REQUIRED)
include_directories(${process_INCLUDE_DIRS})

# output
find_package(output CONFIG REQUIRED)
include_directories(${output_INCLUDE_DIRS})

# HazdevBroker
find_package(HazdevBroker CONFIG REQUIRED)
include_directories(${HazdevBroker_INCLUDE_DIRS})

# librdkafka
set(LIBRDKAFKA_PATH "${CURRENT_SOURCE_DIR}/../../rdkafka/" CACHE PATH "Path to the rdkafka library")
# look for the c lib
find_library(LIBRDKAFKA_C_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)
include_directories(${LIBRDKAFKA_PATH})

# check to see if we found it
if (LIBRDKAFKA_C_LIB STREQUAL "LIBRDKAFKA_C_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka c library")
endif ()
include_directories(${LIBRDKAFKA_PATH}/src)

# look for the cpp lib
find_library(LIBRDKAFKA_CPP_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka++${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)

# check to see if we found it
if (LIBRDKAFKA_CPP_LIB STREQUAL "LIBRDKAFKA_CPP_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka cpp library")
endif ()
include_directories(${LIBRDKAFKA_PATH}/src-cpp)

# uuid
include(${CMAKE_DIR}/uuid.cmake)

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories ("${PROJECT_BINARY_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/input)
include_directories(${PROJECT_SOURCE_DIR}/broker_output)

# ----- LIBRARIES ----- #
# input
add_library (input STATIC ${PROJECT_SOURCE_DIR}/input/input.cpp ${PROJECT_SOURCE_DIR}/input/input.h)

# output
add_library (broker_output STATIC  ${PROJECT_SOURCE_DIR}/broker_output/brokerOutput.cpp ${PROJECT_SOURCE_DIR}/broker_output/brokerOutput.h)

# ----- SET SOURCE FILES ----- #
set(SRCS "${PROJECT_SOURCE_DIR}/glass-broker-app.cpp")

# ----- SET HEADER FILES ----- #
# no headers

# ----- BUILD EXECUTABLE ----- #
# NOTE: Order libraries are linked matters for G++
set(EXE_LIBRARIES input broker_output ${HazdevBroker_LIBRARIES} ${LIBRDKAFKA_CPP_LIB} ${LIBRDKAFKA_C_LIB} ${output_LIBRARIES} ${process_LIBRARIES} ${parse_LIBRARIES} ${DetectionFormats_LIBRARIES} ${glasscore_LIBRARIES} ${util_LIBRARIES} ${config_LIBRARIES} ${log_LIBRARIES} ${SuperEasyJSON_LIBRARIES})
if (UNIX AND NOT APPLE)
    set(ZLIB -lz)
    set(LIBDL -ldl)
endif (UNIX AND NOT APPLE)
include(${CMAKE_DIR}/build_exe.cmake)

# ----- GENERATE DOCUMENTATION ----- #
set(DOC_DIRS "${PROJECT_SOURCE_DIR}/input ${PROJECT_SOURCE_DIR}/broker_output ${PROJECT_SOURCE_DIR}/glass-broker-app.cpp")
include(${CMAKE_DIR}/documentation.cmake)

# ----- RUN CPPCHECK ----- #
include(${CMAKE_DIR}/cppcheck.cmake)

# ----- RUN CPPLINT ----- #
include(${CMAKE_DIR}/cpplint.cmake)

# ----- RUN UNIT TESTS ----- #
# no tests currently

# ----- INSTALL EXECUTABLE ----- #
include(${CMAKE_DIR}/install_exe.cmake)
