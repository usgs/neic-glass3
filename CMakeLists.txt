# neic-glass3 overall CMake configuration file.
#
# This file contains the overall CMake configuration file that builds
# neic-glass3. This configuration follows the "CMake Superbuilds and Git
# Submodules" scheme to organize and orchestrate the build process, as
# described here: https://blog.kitware.com/cmake-superbuilds-git-submodules/
#
# Each local and remote submodule is configured as a separate CMake
# ExternalProject, either by defining a local directory where the submodule
# can be found, or a git repository address. Submodule dependencies are managed
# by ExternalProject's DEPENDS flags plus find_package lookups from within the
# submodule. Build options are passed down to submodules as appropriate.

cmake_minimum_required (VERSION 3.4)

# ----- PROJECT ----- #
project (glass)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# ----- CMAKE INCLUDES ----- #
include(ExternalProject)

# ----- SETUP INSTALL LOCATIONS ----- #
set(INSTALL_LOCATION ${CMAKE_INSTALL_PREFIX})

# ----- PREPEND INSTALL LOCATIONS TO MODULE PATH ----- #
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${INSTALL_LOCATION}" )

# ----- OPTIONS ----- #
option(GENERATE_DOCUMENTATION "Create and install the HTML based API documentation" OFF)
option(RUN_TESTS "Create and run unit tests" ON)
option(BUILD_GLASS-APP "Build the glass application" ON)
option(BUILD_GLASS-BROKER-APP "Build the glass broker based application" OFF)
option(BUILD_GEN-TRAVELTMES-APP "Build the travel time generation application" OFF)
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)
option(RUN_CPPLINT "Run CPP Checks (requires cpplint and python installed)" OFF)
option(SUPPORT_COVERAGE "Instrument for Coverage" OFF)
option(RUN_COVERAGE "Run Coverage Report (requires lcov installed)" OFF)
option(GIT_CLONE_PUBLIC "Clone from public git URLs (https)" OFF)

# ----- EXTERNAL LIBRARIES ----- #
# Google Test
if (RUN_TESTS)
    # ----- LOOK FOR GTEST ----- #
    find_package(GTest)

    # ----- DOWNLOAD GTEST IF NOT FOUND ----- #
    if (NOT GTEST_FOUND)

        set(GTEST_DEPEND "GTest")

        MESSAGE(STATUS "Configuring to download GTest.")
        if (GIT_CLONE_PUBLIC)
          ExternalProject_Add(
              GTest
              GIT_REPOSITORY https://github.com/google/googletest.git
              GIT_TAG release-1.8.0
              TIMEOUT 10
              CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          )
        else()
          ExternalProject_Add(
              GTest
              GIT_REPOSITORY git@github.com:google/googletest.git
              GIT_TAG release-1.8.0
              TIMEOUT 10
              CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          )
        endif()
    endif()
endif(RUN_TESTS)

# cpplint
if (RUN_CPPLINT)
    set(PYTHON_PATH "/usr/bin/python" CACHE FILEPATH "Path to python")
    set(CPPLINT_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/cpplint/cpplint.py" CACHE FILEPATH "Path to cpplint")
else()
    set(PYTHON_PATH "")
    set(CPPLINT_PATH "")
endif(RUN_CPPLINT)

# cppcheck
if (RUN_CPPCHECK)
    set(CPPCHECK_PATH "/usr/local/bin/cppcheck" CACHE FILEPATH "Path to cppcheck")
else()
    set(CPPCHECK_PATH "")
endif(RUN_CPPCHECK)

# Doxygen
if (BUILD_DOCUMENTATION)
    # ----- LOOK FOR DOXYGEN ----- #
    find_package(Doxygen)

    # ----- DOWNLOAD DOXYGEN IF NOT FOUND ----- #
    if(NOT DOXYGEN_FOUND)
        MESSAGE(STATUS "Configuring to download Doxygen.")

        if (GIT_CLONE_PUBLIC)
          ExternalProject_Add(
              Doxygen
              GIT_REPOSITORY https://github.com/doxygen/doxygen.git
              GIT_TAG Release_1_8_12
              TIMEOUT 10
              CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          )
        else()
          ExternalProject_Add(
              Doxygen
              GIT_REPOSITORY git@github.com:doxygen/doxygen.git
              GIT_TAG Release_1_8_12
              TIMEOUT 10
              CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          )
        endif()

        set(DOXYGEN_DEPEND "Doxygen")
    endif()
endif(BUILD_DOCUMENTATION)

# SuperEasyJSON
ExternalProject_Add(
    SuperEasyJSON
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/SuperEasyJSON/
    UPDATE_COMMAND ""
)

if (BUILD_GLASS-APP OR BUILD_GLASS-BROKER-APP)
    # rapidjson
    set(RAPIDJSON_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson" CACHE PATH "Path to rapidjson")

    # DetectionFormats
    if (GIT_CLONE_PUBLIC)
      ExternalProject_Add(DetectionFormats
          GIT_REPOSITORY https://github.com/usgs/earthquake-detection-formats.git
          #GIT_TAG working
          TIMEOUT 10
          CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DRAPIDJSON_PATH=${RAPIDJSON_PATH}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
      )
    else()
      ExternalProject_Add(DetectionFormats
          GIT_REPOSITORY git@github.com:usgs/earthquake-detection-formats.git
          #GIT_TAG working
          TIMEOUT 10
          CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DRAPIDJSON_PATH=${RAPIDJSON_PATH}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
      )
    endif()
endif()

if (BUILD_GLASS-BROKER-APP)
    # rdkafka
    set(LIBRDKAFKA_PATH
        "${CMAKE_CURRENT_LIST_DIR}/lib/rdkafka/"
        CACHE PATH "Path to rdkafka")
    set(LIBRDKAFKA_C_LIB
        "${CMAKE_CURRENT_LIST_DIR}/lib/rdkafka/src/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka${CMAKE_SHARED_LIBRARY_SUFFIX}"
        CACHE FILEPATH "Path to the rdkafka c library")
    set(LIBRDKAFKA_CPP_LIB
        "${CMAKE_CURRENT_LIST_DIR}/lib/rdkafka/src_cpp/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka++${CMAKE_SHARED_LIBRARY_SUFFIX}"
        CACHE FILEPATH "Path to the rdkafka cpp library")

    # HazdevBroker
    if (GIT_CLONE_PUBLIC)
      ExternalProject_Add(HazdevBroker
          GIT_REPOSITORY https://github.com/usgs/hazdev-broker.git
          #GIT_TAG working
          TIMEOUT 10
          CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DRAPIDJSON_PATH=${RAPIDJSON_PATH}
            -DLIBRDKAFKA_PATH=${LIBRDKAFKA_PATH}
            -DLIBRDKAFKA_C_LIB=${LIBRDKAFKA_C_LIB}
            -DLIBRDKAFKA_CPP_LIB=${LIBRDKAFKA_CPP_LIB}
      )
    else()
      ExternalProject_Add(HazdevBroker
          GIT_REPOSITORY git@github.com:usgs/hazdev-broker.git
          #GIT_TAG working
          TIMEOUT 10
          CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DRAPIDJSON_PATH=${RAPIDJSON_PATH}
            -DLIBRDKAFKA_PATH=${LIBRDKAFKA_PATH}
            -DLIBRDKAFKA_C_LIB=${LIBRDKAFKA_C_LIB}
            -DLIBRDKAFKA_CPP_LIB=${LIBRDKAFKA_CPP_LIB}
      )
    endif()
endif()

# ----- PROJECT LIBRARIES ----- #
# glasscore
ExternalProject_Add(
    glasscore
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/glasscore/
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
      -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
      -DRUN_TESTS=${RUN_TESTS}
      -DRUN_CPPCHECK=${RUN_CPPCHECK}
      -DRUN_CPPLINT=${RUN_CPPLINT}
      -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
      -DRUN_COVERAGE=${RUN_COVERAGE}
      -DPYTHON_PATH=${PYTHON_PATH}
      -DCPPLINT_PATH=${CPPLINT_PATH}
      -DCPPCHECK_PATH=${CPPCHECK_PATH}
    DEPENDS SuperEasyJSON ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
    UPDATE_COMMAND ""
)

if (BUILD_GLASS-APP OR BUILD_GLASS-BROKER-APP)

    # log
    ExternalProject_Add(
        log
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/log/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS ${DOXYGEN_DEPEND}
        UPDATE_COMMAND ""
    )

    # util
    ExternalProject_Add(
        util
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/util/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_TESTS=${RUN_TESTS}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DRUN_COVERAGE=${RUN_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON log ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

    # config
    ExternalProject_Add(
        config
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/config/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_TESTS=${RUN_TESTS}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DRUN_COVERAGE=${RUN_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS log util ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

    # parse
    ExternalProject_Add(
        parse
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/parse/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_TESTS=${RUN_TESTS}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DRUN_COVERAGE=${RUN_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON util log DetectionFormats ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

    # process
    ExternalProject_Add(
        process
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/process/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON glasscore util log ${DOXYGEN_DEPEND}
        UPDATE_COMMAND ""
    )

    # output
    ExternalProject_Add(
        output
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/output/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_TESTS=${RUN_TESTS}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DRUN_COVERAGE=${RUN_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON config util log DetectionFormats parse ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

endif()

# ----- PROJECT APPLICATIONS ----- #
if (BUILD_GLASS-APP)

    # glass-app
    ExternalProject_Add(
        glass-app
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/glass-app/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_TESTS=${RUN_TESTS}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DRUN_COVERAGE=${RUN_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON config util log DetectionFormats parse glasscore process output ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

endif()

if (BUILD_GLASS-BROKER-APP)

    # glass-broker-app
    ExternalProject_Add(
        glass-broker-app
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/glass-broker-app/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DLIBRDKAFKA_PATH=${LIBRDKAFKA_PATH}
          -DLIBRDKAFKA_C_LIB=${LIBRDKAFKA_C_LIB}
          -DLIBRDKAFKA_CPP_LIB=${LIBRDKAFKA_CPP_LIB}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON config util log DetectionFormats parse glasscore process output HazdevBroker ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

endif()

if (BUILD_GEN-TRAVELTMES-APP)

    # gen-travel-times-app
    ExternalProject_Add(
        gen-travel-times-app
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/gen-travel-times-app/
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${INSTALL_LOCATION}
          -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
          -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
          -DGENERATE_DOCUMENTATION=${GENERATE_DOCUMENTATION}
          -DRUN_CPPCHECK=${RUN_CPPCHECK}
          -DRUN_CPPLINT=${RUN_CPPLINT}
          -DSUPPORT_COVERAGE=${SUPPORT_COVERAGE}
          -DPYTHON_PATH=${PYTHON_PATH}
          -DCPPLINT_PATH=${CPPLINT_PATH}
          -DCPPCHECK_PATH=${CPPCHECK_PATH}
        DEPENDS SuperEasyJSON config util log glasscore ${DOXYGEN_DEPEND} ${GTEST_DEPEND}
        UPDATE_COMMAND ""
    )

endif()
