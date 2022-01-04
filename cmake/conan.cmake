set(CONAN "conan" CACHE STRING "Conan executable")
set(CONAN_PROFILE "default" CACHE STRING "Conan profile")

include(cmake/conan/conan.cmake)

conan_cmake_run(
  PROFILE ${CONAN_PROFILE}
  SETTINGS build_type=${CMAKE_BUILD_TYPE}
  REQUIRES WasmEdge/0.9.0@sanblch/stable
  GENERATORS cmake cmake_find_package_multi
  CONAN_COMMAND ${CONAN})

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_define_targets()
