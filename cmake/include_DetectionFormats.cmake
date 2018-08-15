# include_DetectionFormats.cmake  - a CMake script that finds the
# DetectionFormats package, and sets up the includes
#
find_package(DetectionFormats CONFIG REQUIRED)
include_directories(${DetectionFormats_INCLUDE_DIRS})
include_directories(${RapidJSON_INCLUDE_DIRS})
