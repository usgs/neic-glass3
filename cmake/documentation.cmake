# documentation.cmake - a script that generates Doxygen documentation for a
# project
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# GENERATE_DOCUMENTATION is a CMake environment variable that sets whether to run
#   Doxygen for this project, defaults to off
# SRCS is a CMake environment variable list that contains the source files
#   to process with Doxygen
# HDRS is a CMake environment variable list that contains the header files
#   to process with Doxygen

# ----- DOXYGEN OPTION ----- #
option(GENERATE_DOCUMENTATION "Runs Doxygen (requires Doxygen installed)" OFF)

# ----- DOXYGEN ----- #
if(GENERATE_DOCUMENTATION)
    # ----- LOOK FOR DOXYGEN ----- #
    # will fail CMake if not found
    find_package(Doxygen REQUIRED)

    # generate doxygen configuration file
    set(doxyfile_in ${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    # run Doxygen after the project build
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    # copy Doxygen output to the distribution directory
    # in the doc subdirectory
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
        DESTINATION doc/${PROJECT_NAME}
    )
endif()
