# ----- CPPCHECK ----- #
option(RUN_CPPCHECK "Run CPP Checks (requires cppcheck installed)" OFF)

if(RUN_CPPCHECK)
    add_custom_command(TARGET ${PROJECT_NAME}
      PRE_BUILD
      COMMAND cppcheck
      --enable=warning,performance,portability
      --language=c++
      --std=c++11
      --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)"
      --verbose
      --suppress=nullPointerRedundantCheck
      --error-exitcode=1
      ${SRCS} ${HDRS}
      COMMENT "Running cppcheck" VERBATIM
    )
endif(RUN_CPPCHECK)
