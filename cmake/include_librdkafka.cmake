# include_HazdevBroker.cmake  - a CMake script that finds the librdkafka
# libraries, and sets up the includes
#
# librdkafka
set(LIBRDKAFKA_PATH "${CURRENT_SOURCE_DIR}/../../rdkafka/" CACHE PATH "Path to the rdkafka library")

# look for the c lib
find_library(LIBRDKAFKA_C_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)
include_directories(${LIBRDKAFKA_PATH})

# check to see if we found it
if (LIBRDKAFKA_C_LIB STREQUAL "LIBRDKAFKA_C_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka c library")
endif ()
include_directories(${LIBRDKAFKA_PATH}/src)

# look for the cpp lib
find_library(LIBRDKAFKA_CPP_LIB
    ${LIBRDKAFKA_PATH}/win32/x64/Release/${CMAKE_SHARED_LIBRARY_PREFIX}rdkafka++${CMAKE_SHARED_LIBRARY_SUFFIX}
    HINTS "${LIBRDKAFKA_PATH}/lib"
)

# check to see if we found it
if (LIBRDKAFKA_CPP_LIB STREQUAL "LIBRDKAFKA_CPP_LIB-NOTFOUND")
    message (FATAL_ERROR "Couldn't find the librdkafka cpp library")
endif ()
include_directories(${LIBRDKAFKA_PATH}/src-cpp)
