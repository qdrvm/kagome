set(CMAKE_BUILD_TYPE Debug)

include(cmake/3rdparty/CodeCoverage.cmake)

append_coverage_compiler_flags()

set(COVERAGE_LCOV_EXCLUDES
  'deps/*'
  'build/*'
  )

setup_target_for_coverage_gcovr_xml(
     NAME ctest_coverage                    # New target name
     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
 )
