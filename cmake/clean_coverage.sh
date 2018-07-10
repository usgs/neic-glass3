#!/bin/sh
# clen_coverage.sh - a bash script that cleans up coverage files before a new
# build
#
# Log current path
currentpath=`pwd`
echo "Current Path: ${currentpath}"
findpath=`which find`

# searches recursivly for .gcda files and removes them
echo "\ndelete old coverage files"
${findpath} . -type f -name "*.gcda" -exec /bin/rm -f {} \;

# Done
echo "\nFinished"
