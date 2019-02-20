if(NOT REPORT_DIR)
  set(REPORT_DIR "${CMAKE_BINARY_DIR}/reports")
  message(STATUS "REPORT_DIR default is ${REPORT_DIR}")
  file(MAKE_DIRECTORY ${REPORT_DIR})
endif()

find_program(LCOV_PROGRAM lcov)
if(NOT LCOV_PROGRAM)
  message(FATAL_ERROR "lcov not found! Aborting...")
endif()
find_file(LCOV_CONFIG_FILE .lcovrc ${PROJECT_SOURCE_DIR})
if(NOT LCOV_CONFIG_FILE)
  message(FATAL_ERROR "lcov config file not found in project root! Aborting...")
endif()
message(STATUS "lcov enabled (${LCOV_PROGRAM})")
# remove -g flag to reduce binary size
list(REMOVE_ITEM CMAKE_CXX_FLAGS -g)
set(CMAKE_CXX_FLAGS "--coverage ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "--coverage ${CMAKE_C_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "--coverage ${CMAKE_SHARED_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "--coverage ${CMAKE_EXE_LINKER_FLAGS}")
add_custom_target(coverage.init.info
  COMMAND ${LCOV_PROGRAM} --config-file ${LCOV_CONFIG_FILE} -o ${PROJECT_BINARY_DIR}/reports/coverage.init.info -c -i -d ${PROJECT_BINARY_DIR}
  )
add_custom_target(coverage.info
  COMMAND ${LCOV_PROGRAM} --config-file ${LCOV_CONFIG_FILE} -o ${PROJECT_BINARY_DIR}/reports/coverage.info -c -d ${PROJECT_BINARY_DIR}
  COMMAND ${LCOV_PROGRAM} --config-file ${LCOV_CONFIG_FILE} -o ${PROJECT_BINARY_DIR}/reports/coverage.info -a ${PROJECT_BINARY_DIR}/reports/coverage.init.info -a ${PROJECT_BINARY_DIR}/reports/coverage.info
  COMMAND ${LCOV_PROGRAM} --config-file ${LCOV_CONFIG_FILE} -o ${PROJECT_BINARY_DIR}/reports/coverage.info -r ${PROJECT_BINARY_DIR}/reports/coverage.info '/usr*' '${CMAKE_SOURCE_DIR}/deps/*'
  )


if(NOT GCOVR_BIN)
  find_program(GCOVR_BIN gcovr)
endif()

if(NOT GCOVR_BIN)
  message(WARNING "gcovr can not be found in PATH. Target gcovr is not available.")
else()
  message(STATUS "Target gcovr enabled")
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(GCOV_BACKEND "llvm-cov gcov")
  else()
    set(GCOV_BACKEND "gcov")
  endif()

  add_custom_target(gcovr
    COMMAND ${GCOVR_BIN} -s -x -r '${CMAKE_SOURCE_DIR}'
    -e '${CMAKE_SOURCE_DIR}/deps/*'
    -e '${CMAKE_BINARY_DIR}/*'
    --gcov-executable='${GCOV_BACKEND}'
    -o ${REPORT_DIR}/gcovr.xml
    COMMENT "Collecting coverage data with gcovr"
    )
endif()
