cmake_minimum_required (VERSION 3.4)

# ----- PROJECT VERSION NUMBER ----- #
set (glass-app_VERSION_MAJOR 0)
set (glass-app_VERSION_MINOR 5)
set (glass-app_VERSION_PATCH 0)

# ----- PROJECT ----- #
project (glass-app VERSION ${glass-app_VERSION_MAJOR}.${glass-app_VERSION_MINOR}.${glass-app_VERSION_PATCH})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/..")

# ----- CMAKE INCLUDES ----- #
include(ExternalProject)
include(internal_utils.cmake)

fix_default_compiler_settings()  # Defined in internal_utils.cmake.

# ----- CMAKE CONFIG HEADER ----- #
# pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/glass-appCMakeConfig.h.in"
    "${PROJECT_BINARY_DIR}/glass-appCMakeConfig.h"
)

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
include_directories(${PROJECT_SOURCE_DIR}/input)
include_directories(${PROJECT_SOURCE_DIR}/file_output)

if (UNIX AND NOT APPLE)
  include_directories(${LIBUUID_INCLUDE_DIR})
endif (UNIX AND NOT APPLE)

# ----- LIBRARIES ----- #
# input
add_library (input STATIC ${PROJECT_SOURCE_DIR}/input/input.cpp ${PROJECT_SOURCE_DIR}/input/input.h)

# output
add_library (file_output STATIC  ${PROJECT_SOURCE_DIR}/file_output/file_output.cpp ${PROJECT_SOURCE_DIR}/file_output/file_output.h)

# ----- EXECUTABLE ----- #
# NOTE: Order libraries are linked matters for G++
add_executable (glass-app ${PROJECT_SOURCE_DIR}/glass-app.cpp)
target_link_libraries(glass-app input)
target_link_libraries(glass-app file_output)
target_link_libraries(glass-app ${output_LIBRARIES})
target_link_libraries(glass-app ${process_LIBRARIES})
target_link_libraries(glass-app ${parse_LIBRARIES})
target_link_libraries(glass-app ${DetectionFormats_LIBRARIES})
target_link_libraries(glass-app ${glasscore_LIBRARIES})
target_link_libraries(glass-app ${util_LIBRARIES})
target_link_libraries(glass-app ${config_LIBRARIES})
target_link_libraries(glass-app ${log_LIBRARIES})
target_link_libraries(glass-app ${SuperEasyJSON_LIBRARIES})

if (UNIX AND NOT APPLE)
    set(PTHREADLIB -pthread)
    target_link_libraries(glass-app ${LIBUUID_LIBRARY})
endif (UNIX AND NOT APPLE)

target_link_libraries(glass-app ${PTHREADLIB} ${GCC_COVERAGE_LINK_FLAGS})

# ----- TESTS ----- #
option(RUN_TESTS "Create and run unit tests (requires GTest)" OFF)

if (RUN_TESTS)

   # ----- LOOK FOR GTEST ----- #
   find_package(GTest REQUIRED)

   enable_testing()

    # ----- TEST SOURCES ----- #
    set (UNITTEST_SOURCES ${PROJECT_SOURCE_DIR}/tests/main.cpp
        ${PROJECT_SOURCE_DIR}/tests/input_unittest.cpp
        ${PROJECT_SOURCE_DIR}/tests/output_unittest.cpp
    )

    # ----- SET TEST INCLUDE DIRECTORIES ----- #
    include_directories(${GTEST_INCLUDE_DIRS})

    # ----- CREATE TEST EXE ----- #
    # NOTE: Order libraries are linked matters for G++
    add_executable(glass-app-tests ${UNITTEST_SOURCES})
    set_target_properties(glass-app-tests PROPERTIES OUTPUT_NAME glass-app-tests)
    target_link_libraries(glass-app-tests input)
    target_link_libraries(glass-app-tests file_output)
    target_link_libraries(glass-app-tests ${output_LIBRARIES})
    target_link_libraries(glass-app-tests ${process_LIBRARIES})
    target_link_libraries(glass-app-tests ${parse_LIBRARIES})
    target_link_libraries(glass-app-tests ${DetectionFormats_LIBRARIES})
    target_link_libraries(glass-app-tests ${glasscore_LIBRARIES})
    target_link_libraries(glass-app-tests ${util_LIBRARIES})
    target_link_libraries(glass-app-tests ${config_LIBRARIES})
    target_link_libraries(glass-app-tests ${log_LIBRARIES})
    target_link_libraries(glass-app-tests ${SuperEasyJSON_LIBRARIES})

    if (UNIX AND NOT APPLE)
        target_link_libraries(glass-app-tests ${LIBUUID_LIBRARY})
    endif (UNIX AND NOT APPLE)

    target_link_libraries(glass-app-tests ${PTHREADLIB} ${GCC_COVERAGE_LINK_FLAGS} ${GTEST_BOTH_LIBRARIES})

    # ----- TESTS ----- #
    GTEST_ADD_TESTS(glass-app-tests "" ${UNITTEST_SOURCES})

    # ----- COPY TEST DATA ----- #
    add_custom_command(TARGET glass-app-tests
        PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/testdata ${CMAKE_CURRENT_BINARY_DIR}/testdata
        COMMENT "Copying Test Data" VERBATIM
    )

    # ----- RUN TESTS ----- #
    add_custom_command(TARGET glass-app-tests
        POST_BUILD
        COMMAND glass-app-tests
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Running glass-app-tests" VERBATIM
    )

    # ----- RUN COVERAGE ----- #
    if(SUPPORT_COVERAGE)

        option(RUN_COVERAGE "Run Coverage Report (requires lcov installed)" OFF)

        if(RUN_COVERAGE)
            add_custom_target(coverage-report ALL
              DEPENDS glass-app-tests
              COMMAND /bin/sh
              ${PROJECT_SOURCE_DIR}/generate_coverage.sh
              COMMENT "Capture/Report Coverage Info" VERBATIM
            )

        endif (RUN_COVERAGE)

    endif (SUPPORT_COVERAGE)
endif()

# ----- CPPCHECK ----- #
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)

if(RUN_CPPCHECK)

    file(GLOB CPPCHECK_SRCS "${PROJECT_SOURCE_DIR}/input/*.h"
                           "${PROJECT_SOURCE_DIR}/input/*.cpp"
                           "${PROJECT_SOURCE_DIR}/file_output/*.h"
                           "${PROJECT_SOURCE_DIR}/file_output/*.cpp"
                           "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cppcheck ALL
      DEPENDS glass-app
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
                           "${PROJECT_SOURCE_DIR}/file_output/*.h"
                           "${PROJECT_SOURCE_DIR}/file_output/*.cpp"
                           "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cpplint ALL
      DEPENDS glass-app
      COMMAND /usr/bin/python "${CPPLINT_PATH}"
      --filter=-whitespace/tab,-legal/copyright,-build/c++11,-build/header_guard,-readability/fn_size
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
install(TARGETS glass-app DESTINATION "${PROJECT_NAME}")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/params DESTINATION "${PROJECT_NAME}")
