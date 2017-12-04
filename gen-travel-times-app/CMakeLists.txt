cmake_minimum_required (VERSION 3.4)

# ----- PROJECT VERSION NUMBER ----- #
set (gen-travel-times-app_VERSION_MAJOR 0)
set (gen-travel-times-app_VERSION_MINOR 1)
set (gen-travel-times-app_VERSION_PATCH 0)

# ----- PROJECT ----- #
project (gen-travel-times-app VERSION ${gen-travel-times-app_VERSION_MAJOR}.${gen-travel-times-app_VERSION_MINOR}.${gen-travel-times-app_VERSION_PATCH})

# ----- CMAKE INCLUDES ----- #
include(ExternalProject)
include(internal_utils.cmake)

fix_default_compiler_settings()  # Defined in internal_utils.cmake.

# ----- CMAKE CONFIG HEADER ----- #
# pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/gen-travel-times-appCMakeConfig.h.in"
    "${PROJECT_BINARY_DIR}/gen-travel-times-appCMakeConfig.h"
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

# ----- LIBRARIES ----- #

# ----- EXECUTABLE ----- #
add_executable (gen-travel-times-app ${PROJECT_SOURCE_DIR}/gen-travel-times-app.cpp)
target_link_libraries(gen-travel-times-app ${SuperEasyJSON_LIBRARIES})
target_link_libraries(gen-travel-times-app ${glasscore_LIBRARIES})
target_link_libraries(gen-travel-times-app ${log_LIBRARIES})
target_link_libraries(gen-travel-times-app ${config_LIBRARIES})
target_link_libraries(gen-travel-times-app ${util_LIBRARIES})

# ----- TESTS ----- #
# no tests currently

# ----- CPPCHECK ----- #
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)

if(RUN_CPPCHECK)

    file(GLOB CPPCHECK_SRCS "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cppcheck ALL
      DEPENDS gen-travel-times-app
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
    file(GLOB CPPLINT_SRCS "${PROJECT_SOURCE_DIR}/*.cpp")

    add_custom_target(cpplint ALL
      DEPENDS gen-travel-times-app
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
install(TARGETS gen-travel-times-app DESTINATION "${PROJECT_NAME}")
install(DIRECTORY ${PROJECT_SOURCE_DIR}/params DESTINATION "${PROJECT_NAME}")
