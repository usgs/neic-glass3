# cpplint.cmake - a script that runs cpp linter (code style) checks on the
# project prior to the build.
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# RUN_CPPLINT is a CMake environment variable that sets whether to run
#   cpplint for this project, defaults to off
# PYTHON_PATH is a CMake environment variable that defines the path to the
#   python executable
# CPPLINT_PATH is a CMake environment variable that defines the path to the
#   cpplint python script
# SRCS is a CMake environment variable list that contains the source files
#   to check with cpplint
# HDRS is a CMake environment variable list that contains the header files
#   to check with cpplint

# ----- CPPLINT OPTION ----- #
option(RUN_CPPLINT "Run CPP Linter (requires cpplint and python installed)" OFF)

# ----- CPPLINT ----- #
if(RUN_CPPLINT)
    # set path defaults
    set(PYTHON_PATH "/usr/bin/python" CACHE FILEPATH "Path to python")
    set(CPPLINT_PATH "${CURRENT_SOURCE_DIR}/lib/cpplint/cpplint.py" CACHE FILEPATH "Path to cpplint")

    # run cpplint before the project build
    add_custom_command(TARGET ${PROJECT_NAME}
      PRE_BUILD
      COMMAND "${PYTHON_PATH}" "${CPPLINT_PATH}"
      --filter=-whitespace/tab,-legal/copyright,-build/c++11,-build/header_guard,-readability/fn_size
      ${SRCS} ${HDRS}
      COMMENT "Running cpplint" VERBATIM
    )
endif(RUN_CPPLINT)
