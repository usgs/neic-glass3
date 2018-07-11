# build_lib.cmake - a script that builds a library file for a project
# (such as util)
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# SRCS is a CMake environment variable list that contains the source files
#   for the library
# HDRS is a CMake environment variable list that contains the header files
#   for the library

# ----- CREATE STATIC LIBRARY ----- #
add_library (${PROJECT_NAME} STATIC ${SRCS} ${HDRS})

# ----- RENAME LIBRARY ----- #
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME})

# ----- GENERATE EXPORT HEADER ----- #
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h)

# ----- GLOBAL INCLUDES ----- #
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)
