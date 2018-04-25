cmake_minimum_required (VERSION 3.4)

# ----- PROJECT VERSION NUMBER ----- #
set (glass-broker-app_VERSION_MAJOR 0)
set (glass-broker-app_VERSION_MINOR 5)
set (glass-broker-app_VERSION_PATCH 1)

# ----- PROJECT ----- #
project (glass-broker-app VERSION ${glass-broker-app_VERSION_MAJOR}.${glass-broker-app_VERSION_MINOR}.${glass-broker-app_VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/..")

# ----- CMAKE INCLUDES ----- #
include(ExternalProject)
include(internal_utils.cmake)

fix_default_compiler_settings()  # Defined in internal_utils.cmake.

# ----- CMAKE CONFIG HEADER ----- #
# pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/glass-broker-appCMakeConfig.h.in"
    "${PROJECT_BINARY_DIR}/glass-broker-appCMakeConfig.h"
)
# add the binary tree to the search path for include files
# so that we will find GlassCMakeConfig.h
include_directories ("${PROJECT_BINARY_DIR}")

# ----- NON WINDOWS CONFIG ----- #
if (NOT MSVC)
    # C++ 14 Standard
    SET(GCC_CXX_14_FLAGS "-std=c++14")
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CXX_14_FLAGS} ")

    option(SUPPORT_COVERAGE "Instrument for Coverage" OFF)

    if(SUPPORT_COVERAGE)
        # Coverage
        SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
        SET(GCC_COVERAGE_LINK_FLAGS "--coverage")

        # set flags
        SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CXX_14_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
    endif (SUPPORT_COVERAGE)
endif (NOT MSVC)

# ----- EXTERNAL LIBRARIES ----- #
# SuperEasyJSON
find_package(SuperEasyJSON REQUIRED)

# glasscore
find_package(glasscore CONFIG REQUIRED)

# log
find_package(log CONFIG REQUIRED)

# config
find_package(config CONFIG REQUIRED)

# util
find_package(util CONFIG REQUIRED)

# detection-formats
find_package(DetectionFormats CONFIG REQUIRED)

# parse
find_package(parse CONFIG REQUIRED)

# process
find_package(process CONFIG REQUIRED)

# output
find_package(output CONFIG REQUIRED)

# HazdevBroker
find_package(HazdevBroker CONFIG REQUIRED)

# librdkafka
set(LIBRDKAFKA_PATH "${CURRENT_SOURCE_DIR}/../../rdkafka/" CACHE PATH "Path to the rdkafka library")
# look for the c lib
find_library(LIBRDKAFKA_C_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)

# check to see if we found it
if (LIBRDKAFKA_C_LIB STREQUAL "LIBRDKAFKA_C_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka c library")
endif ()

# look for the cpp lib
find_library(LIBRDKAFKA_CPP_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka++${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)

# check to see if we found it
if (LIBRDKAFKA_CPP_LIB STREQUAL "LIBRDKAFKA_CPP_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka cpp library")
endif ()

# uuid
if (UNIX AND NOT APPLE)
    find_package(Libuuid REQUIRED)
      if (NOT LIBUUID_FOUND)
        message(FATAL_ERROR
          "You might need to run 'sudo apt-get install uuid-dev' or similar")
      endif()
endif (UNIX AND NOT APPLE)

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories ("${PROJECT_BINARY_DIR}")
include_directories(${SuperEasyJSON_INCLUDE_DIRS})
include_directories(${glasscore_INCLUDE_DIRS}/glasslib/include)
include_directories(${glasscore_INCLUDE_DIRS}/glassutil/include)
include_directories(${glasscore_INCLUDE_DIRS}/traveltime/include)
include_directories(${log_INCLUDE_DIRS})
include_directories(${spdlog_INCLUDE_DIRS}/..)
include_directories(${spdlog_INCLUDE_DIRS})
include_directories(${config_INCLUDE_DIRS})
include_directories(${util_INCLUDE_DIRS})
include_directories(${DetectionFormats_INCLUDE_DIRS})
include_directories(${RapidJSON_INCLUDE_DIRS})
include_directories(${parse_INCLUDE_DIRS})
include_directories(${process_INCLUDE_DIRS})
include_directories(${output_INCLUDE_DIRS})
include_directories(${HazdevBroker_INCLUDE_DIRS})
include_directories(${LIBRDKAFKA_PATH})
include_directories(${LIBRDKAFKA_PATH}/src)
include_directories(${LIBRDKAFKA_PATH}/src-cpp)
include_directories(${PROJECT_SOURCE_DIR}/input)
include_directories(${PROJECT_SOURCE_DIR}/broker_output)

if (UNIX AND NOT APPLE)
  include_directories(${LIBUUID_INCLUDE_DIR})
endif (UNIX AND NOT APPLE)

# ----- LIBRARIES ----- #
# input
add_library (input STATIC ${PROJECT_SOURCE_DIR}/input/input.cpp ${PROJECT_SOURCE_DIR}/input/input.h)

# output
add_library (broker_output STATIC  ${PROJECT_SOURCE_DIR}/broker_output/brokerOutput.cpp ${PROJECT_SOURCE_DIR}/broker_output/brokerOutput.h)

if (UNIX AND NOT APPLE)
    set(PTHREADLIB -pthread)
    set(ZLIB -lz)
    set(LIBDL -ldl)
endif (UNIX AND NOT APPLE)

# ----- EXECUTABLE ----- #
# NOTE: Order libraries are linked matters for G++
add_executable (glass-broker-app ${PROJECT_SOURCE_DIR}/glass-broker-app.cpp)
target_link_libraries(glass-broker-app input)
target_link_libraries(glass-broker-app broker_output)
target_link_libraries(glass-broker-app ${HazdevBroker_LIBRARIES})
target_link_libraries(glass-broker-app ${LIBRDKAFKA_CPP_LIB})
target_link_libraries(glass-broker-app ${LIBRDKAFKA_C_LIB})
target_link_libraries(glass-broker-app ${output_LIBRARIES})
target_link_libraries(glass-broker-app ${process_LIBRARIES})
target_link_libraries(glass-broker-app ${parse_LIBRARIES})
target_link_libraries(glass-broker-app ${DetectionFormats_LIBRARIES})
target_link_libraries(glass-broker-app ${glasscore_LIBRARIES})
target_link_libraries(glass-broker-app ${util_LIBRARIES})
target_link_libraries(glass-broker-app ${config_LIBRARIES})
target_link_libraries(glass-broker-app ${log_LIBRARIES})
target_link_libraries(glass-broker-app ${SuperEasyJSON_LIBRARIES})

if (UNIX AND NOT APPLE)
    set(PTHREADLIB -pthread)
    target_link_libraries(glass-broker-app ${LIBUUID_LIBRARY})
endif (UNIX AND NOT APPLE)

target_link_libraries(glass-broker-app ${PTHREADLIB} ${GCC_COVERAGE_LINK_FLAGS} ${ZLIB} ${LIBDL})

# ----- TESTS ----- #
# no tests currently

# ----- CPPCHECK ----- #
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)

if(RUN_CPPCHECK)

    file(GLOB CPPCHECK_SRCS "${PROJECT_SOURCE_DIR}/input/*.h"
                           "${PROJECT_SOURCE_DIR}/input/*.cpp"
                           "${PROJECT_SOURCE_DIR}/broker_output/*.h"
                           "${PROJECT_SOURCE_DIR}/broker_output/*.cpp"
                           "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cppcheck ALL
      DEPENDS glass-broker-app
      COMMAND cppcheck
      --enable=warning,performance,portability
      --language=c++
      --std=c++11
      --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
      --verbose
      --suppress=nullPointerRedundantCheck
      --error-exitcode=1
      ${CPPCHECK_SRCS}
      COMMENT "Running cppcheck" VERBATIM
    )
endif()

# ----- CPPLINT ----- #
option(RUN_CPPLINT "Run CPP Linter (requires cpplint and python installed)" OFF)

if(RUN_CPPLINT)

    set(CPPLINT_PATH "${CURRENT_SOURCE_DIR}/lib/cpplint/cpplint.py" CACHE FILEPATH "Path to cpplint")
    file(GLOB CPPLINT_SRCS "${PROJECT_SOURCE_DIR}/input/*.h"
                           "${PROJECT_SOURCE_DIR}/input/*.cpp"
                           "${PROJECT_SOURCE_DIR}/broker_output/*.h"
                           "${PROJECT_SOURCE_DIR}/broker_output/*.cpp"
                           "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cpplint ALL
      DEPENDS glass-broker-app
      COMMAND /usr/bin/python "${CPPLINT_PATH}"
      --filter=-whitespace/tab,-legal/copyright,-build/c++11,-build/header_guard,-readability/fn_size,-build/include
      ${CPPLINT_SRCS}
      COMMENT "Running cpplint" VERBATIM
    )
endif()

# ----- DOCUMENTATION ----- #
option(GENERATE_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" OFF)

if(GENERATE_DOCUMENTATION)
    # ----- LOOK FOR DOXYGEN ----- #
    find_package(Doxygen REQUIRED)

    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION ${PROJECT_NAME}/doc)
endif()

# ----- INSTALL ----- #
install(TARGETS glass-broker-app DESTINATION "${PROJECT_NAME}")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/params DESTINATION "${PROJECT_NAME}")
