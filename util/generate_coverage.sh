#!/bin/sh
currentpath=`pwd`

echo "Current Path: ${currentpath}"
echo "\ncapture coverage info"
lcov --directory . --capture --output-file coverage.info
echo "\nfilter out system libraries"
lcov --remove coverage.info '/usr/*' '/Applications/*' --output-file coverage.info
echo "\nfilter out external libraries"
lcov --remove coverage.info '*distribution/*' --output-file coverage.info
echo "\nfilter out tests"
lcov --remove coverage.info '*tests/*' --output-file coverage.info
echo "\nprint coverage report"
lcov --list coverage.info
