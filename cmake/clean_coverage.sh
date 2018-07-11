#!/bin/sh
# clen_coverage.sh - a bash script that cleans up coverage files before a new
# build
#
# Log current path
currentpath=`pwd`
echo "Current Path: ${currentpath}"

# searches recursivly for .gcda files and removes them
echo "\ndelete old coverage files"
# find . -type f -name "*.gcda"
find . -type f -name "*.gcda" -exec /bin/rm -vf {} \;

# Done
echo "\nFinished"
