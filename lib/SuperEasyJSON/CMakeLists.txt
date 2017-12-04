cmake_minimum_required (VERSION 3.4)

# ----- PROJECT ----- #
project (SuperEasyJSON VERSION 2.2.0)

# ----- CMAKE INCLUDES ----- #
include(internal_utils.cmake)

fix_default_compiler_settings()  # Defined in internal_utils.cmake.

# ----- SET FOR C++14 STANDARD ----- #
if (NOT MSVC)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif ()

# ----- SET INCLUDE DIRECTORIES ----- #
include_directories(${PROJECT_SOURCE_DIR})

# ----- SET SOURCE FILES ----- #
set (SRCS ${SRCS} ${PROJECT_SOURCE_DIR}/json.cpp
)

# ----- SET HEADER FILES ----- #
set (HDRS ${HDRS} ${PROJECT_SOURCE_DIR}/json.h
)

# ----- CREATE LIBRARY ----- #
add_library (SuperEasyJSON STATIC ${SRCS} ${HDRS})

# ----- TARGET PROPERTIES ----- #
set_target_properties(SuperEasyJSON PROPERTIES
    OUTPUT_NAME SuperEasyJSON
)

# ----- GENERATE ----- #
include(GenerateExportHeader)
generate_export_header(SuperEasyJSON EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/SuperEasyJSON_export.h)

# ----- GLOBAL INCLUDES ----- #
# Note:
#   * header location in project: json.h
#   * header can be included by C++ code `#include <json.h>`
#   * header location in project: ${CMAKE_CURRENT_BINARY_DIR}/SuperEasyJSON_export.hpp
#   * header can be included by: `#include <SuperEasyJSON_export.hpp>`
target_include_directories(
    SuperEasyJSON PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
)

# ----- INSTALL RULES ----- #
# Layout. This works for all platforms:
#   * <prefix>/lib/<PROJECT-NAME>
#   * <prefix>/lib/
#   * <prefix>/include/
set(config_install_dir "lib/${PROJECT_NAME}")
set(include_install_dir "include")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# ----- Configuration ----- #
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with function 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Note: PROJECT_VERSION is used as a VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * targets_export_name
#   * PROJECT_NAME
configure_package_config_file(
    "Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

# Targets:
#   * <prefix>/lib/libSuperEasyJSON.a
#   * header location after install: <prefix>/include/json.h
#   * headers can be included by C++ code `#include <json.h>`
install(
    TARGETS SuperEasyJSON
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * json.h-> <prefix>/include/json.h
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}
    DESTINATION "${include_install_dir}"
    FILES_MATCHING PATTERN "*.h"
)

# Export headers:
#   * ${CMAKE_CURRENT_BINARY_DIR}/SuperEasyJSON_export.h -> <prefix>/include/SuperEasyJSON_export.h
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/SuperEasyJSON_export.h"
    DESTINATION "${include_install_dir}"
)

# Config
#   * <prefix>/lib/SuperEasyJSON/SuperEasyJSONConfig.cmake
#   * <prefix>/lib/SuperEasyJSON/SuperEasyJSONConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config
#   * <prefix>/lib/SuperEasyJSON/SuperEasyJSONTargets.cmake
install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
