# build_exe.cmake - a CMake script that builds an executable file for a project
# (such as glass-app)
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# SRCS is a CMake environment variable list that contains the source files
#   for the executable
# HDRS is a CMake environment variable list that contains the header files
#   for the executable
# EXE_LIBRARIES is a CMake environment variable list that contains the libraries
#   to link to this executable
# LIBUUID_LIBRARY is a CMake environment variable that identifies the uuid
#   library location (only defined for linux builds)
# PTHREADLIB is a CMake environment variable that identifies the threading
#   library location (only defined for builds where google testing is enabled)
# ZLIB is a CMake environment variable that identifies the zlib
#   library location (only defined for glass-broker-app builds)
# LIBDL is a CMake environment variable that sets the linker to use the
#   dynamic libraries (only set for glass-broker-app builds)
# GCC_COVERAGE_LINK_FLAGS is a CMake environment variable that sets the linker
#   flags to support code coverage (only set for builds where code coverage is
#   enabled)

# ----- BUILD EXE ----- #
add_executable (${PROJECT_NAME} ${SRCS} ${HDRS})

# ----- LINK LIBS ----- #
target_link_libraries(${PROJECT_NAME} ${EXE_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${LIBUUID_LIBRARY} ${PTHREADLIB} ${ZLIB} ${LIBDL} ${GCC_COVERAGE_LINK_FLAGS})
