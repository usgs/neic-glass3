# ----- BUILD EXE ----- #
add_executable (${PROJECT_NAME} ${SRCS} ${HDRS})
target_link_libraries(${PROJECT_NAME}  ${EXE_LIBRARIES})

if (UNIX AND NOT APPLE)
    set(PTHREADLIB -pthread)
endif (UNIX AND NOT APPLE)

target_link_libraries(${PROJECT_NAME} ${LIBUUID_LIBRARY} ${PTHREADLIB} ${ZLIB} ${LIBDL} ${GCC_COVERAGE_LINK_FLAGS})
