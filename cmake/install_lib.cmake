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
    "${CMAKE_CURRENT_LIST_DIR}/package_config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

# Targets:
#   * <prefix>/lib/libconfig.a
#   * header location after install: <prefix>/include/*.h
install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * *.h-> <prefix>/include/*.h
install(
    FILES ${HDRS}
      DESTINATION "${include_install_dir}/${PROJECT_NAME}"
)

# Export headers:
#   * ${CMAKE_CURRENT_BINARY_DIR}/config_export.h -> <prefix>/include/config_export.h
install(
FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_export.h"
    DESTINATION "${include_install_dir}"
)

# Config
#   * <prefix>/lib/config/configConfig.cmake
#   * <prefix>/lib/config/configConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config
#   * <prefix>/lib/config/configTargets.cmake
install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
