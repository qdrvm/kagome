if(NOT CLANG_TIDY_BIN)
  find_program(CLANG_TIDY_BIN
    NAMES clang-tidy clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17 clang-tidy-16
    DOC "Path to clang-tidy executable"
  )
endif()

if(NOT CLANG_TIDY_BIN)
  message(FATAL_ERROR "clang-tidy is not installed. Aborting...")
else()
  message(STATUS "clang-tidy has been found: ${CLANG_TIDY_BIN}")
endif()

set(CMAKE_C_CLANG_TIDY   ${CLANG_TIDY_BIN})
set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_BIN})
