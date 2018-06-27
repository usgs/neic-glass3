# install_exe.cmake - a script that installs an executable and configuration
# file(s) for a project
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.

# ----- INSTALL EXE ----- #
install(TARGETS ${PROJECT_NAME} DESTINATION "${PROJECT_NAME}")

# ----- COPY PARAMS ----- #
install(DIRECTORY ${PROJECT_SOURCE_DIR}/params DESTINATION "${PROJECT_NAME}")
