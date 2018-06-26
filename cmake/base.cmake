# base.cmake - a CMake script that sets up basic complier settings and flags
# for a project.

# ----- WINDOWS CONFIG ----- #
if (MSVC)
  # For MSVC, CMake sets certain flags to defaults we want to override.
  # This replacement code is taken from sample in the CMake Wiki at
  # http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace.
  foreach (flag_var
           CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
           CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if (NOT BUILD_SHARED_LIBS)
      # When built as a shared library, it should also use
      # shared runtime libraries.  Otherwise, it may end up with multiple
      # copies of runtime library data in different modules, resulting in
      # hard-to-find crashes. When it is built as a static library, it is
      # preferable to use CRT as static libraries, as we don't have to rely
      # on CRT DLLs being available. CMake always defaults to using shared
      # CRT libraries, so we override that default here.
      string(REPLACE "/MD" "-MT" ${flag_var} "${${flag_var}}")
    endif()
  endforeach()
endif()

# ----- NON WINDOWS CONFIG ----- #
if (NOT MSVC)
    # C++ 14 Standard Flags
    SET(GCC_CXX_14_FLAGS "-std=c++14")

    # Add C++ 14 flag to compiler flags
    SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_CXX_14_FLAGS} ")

    # ----- CODE COVERAGE -----#
    option(SUPPORT_COVERAGE "Instrument for Coverage" OFF)
    if(SUPPORT_COVERAGE)
        # Coverage flags
        SET(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
        SET(GCC_COVERAGE_LINK_FLAGS "--coverage")

        # Add coverage flags to compiler and linker flags
        SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
    endif (SUPPORT_COVERAGE)
endif (NOT MSVC)

# ----- LINUX CONFIG ----- #
if (UNIX AND NOT APPLE)
    set(PTHREADLIB -pthread)
endif (UNIX AND NOT APPLE)

# ----- VERSION HEADER ----- #
# Pass the project version to the source code via a derived header
# file generated from project_version.h.in
configure_file (
    "${CMAKE_CURRENT_LIST_DIR}/project_version.h.in"
    "${PROJECT_BINARY_DIR}/project_version.h"
)
