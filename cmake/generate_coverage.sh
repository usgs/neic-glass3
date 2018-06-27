#!/bin/sh
# generate_coverage.sh - a bash script that generates a coverage report, and
# filters out various system and external libraries, used by test.cmake
#
# Log current path
currentpath=`pwd`
echo "Current Path: ${currentpath}"

# process current directory for coverage information
echo "\ncapture coverage info"
lcov --directory . --capture --output-file coverage.info

# filter out things we don't want
echo "\nfilter out system libraries"
lcov --remove coverage.info '/usr/*' '/Applications/*' --output-file coverage.info
echo "\nfilter out external libraries"
lcov --remove coverage.info '*distribution/*' --output-file coverage.info
lcov --remove coverage.info '*spdlog/*' --output-file coverage.info
lcov --remove coverage.info '*SuperEasyJSON/*' --output-file coverage.info
echo "\nfilter out tests"
lcov --remove coverage.info '*tests/*' --output-file coverage.info

# print the results
echo "\nprint coverage report"
lcov --list coverage.info
