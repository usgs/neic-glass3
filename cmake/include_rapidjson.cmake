# include_rapidjson.cmake  - a CMake script that finds the rapidjson
# include files
#
# rapidjson
set(RAPIDJSON_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/rapidjson" CACHE PATH "Path to rapidjson")

include_directories(${RAPIDJSON_PATH})
