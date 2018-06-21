# ----- CMAKE INCLUDES ----- #
include(${CMAKE_CURRENT_LIST_DIR}/internal_utils.cmake)

fix_default_compiler_settings()  # Defined in internal_utils.cmake.

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

# ----- CMAKE CONFIG HEADER ----- #
# pass some of the CMake settings
# to the source code
configure_file (
    "${CMAKE_CURRENT_LIST_DIR}/project_version.h.in"
    "${PROJECT_BINARY_DIR}/project_version.h"
)
