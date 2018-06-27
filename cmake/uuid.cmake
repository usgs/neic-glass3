# uuid.cmake - a CMake script that handles uuid on linux distributions

# only on linux
if (UNIX AND NOT APPLE)
    # add the current directory to CMAKE_MODULE_PATH so that we can get at
    # FindLibuuid.cmake
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")
    find_package(Libuuid REQUIRED)

    # make sure we got uuid
    if (NOT LIBUUID_FOUND)
        message(FATAL_ERROR
          "You might need to run 'sudo apt-get install uuid-dev' or similar")
    endif()

    # add to our include directories
    include_directories(${LIBUUID_INCLUDE_DIR})
endif (UNIX AND NOT APPLE)
