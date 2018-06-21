# ----- CREATE LIBRARY ----- #
add_library (${PROJECT_NAME} STATIC ${SRCS} ${HDRS})

# ----- TARGET PROPERTIES ----- #
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME})

# ----- GENERATE ----- #
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME} EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h)

# ----- GLOBAL INCLUDES ----- #
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)
