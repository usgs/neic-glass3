# test.cmake - a CMake script for running unit tests, generating coverage
# information, and using generate_coverage.sh to create a coverage report for
# a project
#
# PROJECT_NAME is a CMake environment variable that contains the name of the
#   project.
# RUN_TESTS is a CMake environment variable that sets whether to run
#   unit tests for this project, defaults to off
# TESTS is a CMake environment variable list that contains the unit test source
#   files to test with gtest
# LIBUUID_LIBRARY is a CMake environment variable that identifies the uuid
#   library location (only defined for linux builds)
# TEST_LIBRARIES is a CMake environment variable that identifies the other
#   project libraries that the unit tests require

# ----- TESTS OPTION ----- #
option(RUN_TESTS "Create and run unit tests (requires GTest)" OFF)

# ----- TESTS ----- #
if (RUN_TESTS)
    # make sure we have something to test
    if (TESTS)
        # ----- LOOK FOR GTEST ----- #
        find_package(GTest REQUIRED)

        # enable the CMake testing engine
        enable_testing()

        # ----- SET TEST INCLUDE DIRECTORIES ----- #
        include_directories(${GTEST_INCLUDE_DIRS})

        # ----- CREATE TEST EXE ----- #
        add_executable(${PROJECT_NAME}-unit-tests ${TESTS})
        set_target_properties(${PROJECT_NAME}-unit-tests PROPERTIES OUTPUT_NAME ${PROJECT_NAME}-unit-tests)

        # ----- LINK LIBRARIES ----- #
        # NOTE: Order libraries are linked matters for G++
        # link the gtest libraries
        target_link_libraries(${PROJECT_NAME}-unit-tests ${GTEST_BOTH_LIBRARIES})

        # link the uuid library (only defined on linux)
        target_link_libraries(${PROJECT_NAME}-unit-tests ${LIBUUID_LIBRARY})

        # Link the target library (if the target is a library)
        get_target_property(target_type ${PROJECT_NAME} TYPE)
        if (NOT "${target_type}" STREQUAL "EXECUTABLE")
            target_link_libraries(${PROJECT_NAME}-unit-tests ${PROJECT_NAME})
        endif ()

        # link various libraries we might be dependent on
        target_link_libraries(${PROJECT_NAME}-unit-tests ${TEST_LIBRARIES})

        # link various optional libraries and flags
        target_link_libraries(${PROJECT_NAME}-unit-tests ${PTHREADLIB} ${GCC_COVERAGE_LINK_FLAGS})

        # ----- ADD TESTS TO TEST ENGINE ----- #
        GTEST_ADD_TESTS(${PROJECT_NAME}-unit-tests "" ${TESTS})

        # ----- COPY TEST DATA ----- #
        add_custom_command(TARGET ${PROJECT_NAME}-unit-tests
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/testdata ${CMAKE_CURRENT_BINARY_DIR}/testdata
            COMMENT "Copying Test Data" VERBATIM
        )

        # ----- RUN TESTS ----- #
        add_custom_command(TARGET ${PROJECT_NAME}-unit-tests
            POST_BUILD
            COMMAND ${PROJECT_NAME}-unit-tests
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running ${PROJECT_NAME}-unit-tests" VERBATIM
        )

        # make sure we support coverage
        if(SUPPORT_COVERAGE)
            # ----- RUN COVERAGE OPTION----- #
            option(RUN_COVERAGE "Run Coverage Report (requires lcov installed)" OFF)

            # ----- RUN COVERAGE ----- #
            if(RUN_COVERAGE)
                  # run coverage after the unit test build
                  add_custom_command(TARGET ${PROJECT_NAME}-unit-tests
                  POST_BUILD
                  COMMAND /bin/sh
                  ${CMAKE_CURRENT_LIST_DIR}/generate_coverage.sh
                  COMMENT "Capture/Report Coverage Info" VERBATIM
                )
            endif (RUN_COVERAGE)
        endif (SUPPORT_COVERAGE)
    endif (TESTS)
endif(RUN_TESTS)
