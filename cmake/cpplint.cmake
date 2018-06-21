# ----- CPPLINT ----- #
option(RUN_CPPLINT "Run CPP Linter (requires cpplint and python installed)" OFF)

if(RUN_CPPLINT)
    set(CPPLINT_PATH "${CURRENT_SOURCE_DIR}/lib/cpplint/cpplint.py" CACHE FILEPATH "Path to cpplint")

    add_custom_command(TARGET ${PROJECT_NAME}
      PRE_BUILD
      COMMAND /usr/bin/python "${CPPLINT_PATH}"
      --filter=-whitespace/tab,-legal/copyright,-build/c++11,-build/header_guard,-readability/fn_size
      ${SRCS} ${HDRS}
      COMMENT "Running cpplint" VERBATIM
    )
endif(RUN_CPPLINT)
