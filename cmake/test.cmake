# ----- TESTS ----- #
option(RUN_TESTS "Create and run unit tests (requires GTest)" OFF)

if (RUN_TESTS)
    if (TESTS)
        # ----- LOOK FOR GTEST ----- #
        find_package(GTest REQUIRED)

        enable_testing()

        if (UNIX AND NOT APPLE)
            set(PTHREADLIB -pthread)
        endif (UNIX AND NOT APPLE)

        # ----- SET TEST INCLUDE DIRECTORIES ----- #
        include_directories(${GTEST_INCLUDE_DIRS})

        # ----- CREATE TEST EXE ----- #
        add_executable(${PROJECT_NAME}-unit-tests ${TESTS})
        set_target_properties(${PROJECT_NAME}-unit-tests PROPERTIES OUTPUT_NAME ${PROJECT_NAME}-unit-tests)
        target_link_libraries(${PROJECT_NAME}-unit-tests ${LIBUUID_LIBRARY})
        target_link_libraries(${PROJECT_NAME}-unit-tests ${TEST_LIBRARIES})
        target_link_libraries(${PROJECT_NAME}-unit-tests ${PTHREADLIB} ${GCC_COVERAGE_LINK_FLAGS} ${GTEST_BOTH_LIBRARIES})

        # Cannot link tests to exe targets
        get_target_property(target_type ${PROJECT_NAME} TYPE)
        if (NOT "${target_type}" STREQUAL "EXECUTABLE")
            target_link_libraries(${PROJECT_NAME}-unit-tests ${PROJECT_NAME})
        endif ()

        # ----- TESTS ----- #
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

        # ----- RUN COVERAGE ----- #
        if(SUPPORT_COVERAGE)
            option(RUN_COVERAGE "Run Coverage Report (requires lcov installed)" OFF)

            if(RUN_COVERAGE)
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
