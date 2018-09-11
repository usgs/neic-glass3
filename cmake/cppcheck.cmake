# cppcheck.cmake - a script that runs cpp error checks for a project prior to
# the build.
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# RUN_CPPCHECK is a CMake environment variable that sets whether to run
#   cppcheck for this project, defaults to off
# CPPCHECK_PATH is a CMake environment variable that defines the path to the
#   cppcheck executable
# SRCS is a CMake environment variable list that contains the source files
#   to check with cppcheck
# HDRS is a CMake environment variable list that contains the header files
#   to check with cppcheck

# ----- CPPCHECK OPTION ----- #
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)

# ----- CPPCHECK ----- #
if(RUN_CPPCHECK)
    # set path defaults
    set(CPPCHECK_PATH "/usr/local/bin/cppcheck" CACHE FILEPATH "Path to cppcheck")

    # run cppcheck before the project build
    add_custom_command(TARGET ${PROJECT_NAME}
      PRE_BUILD
      COMMAND "${CPPCHECK_PATH}"
      --enable=warning,performance,portability
      --language=c++
      --std=c++11
      --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
      --verbose
      --suppress=nullPointerRedundantCheck
      --error-exitcode=1
      ${SRCS} ${HDRS}
      COMMENT "Running cppcheck" VERBATIM
    )
endif(RUN_CPPCHECK)
