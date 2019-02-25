set(prefix ${CMAKE_BINARY_DIR}/deps)
set(source_dir ${prefix}/src/binaryen)
set(binary_dir ${prefix}/src/binaryen-build)
# Use source dir because binaryen only installs single header with C API.
set(binaryen_include_dir ${source_dir}/src)
set(binaryen_library ${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}binaryen${CMAKE_STATIC_LIBRARY_SUFFIX})

# Include also other static libs needed:
set(binaryen_other_libraries
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}passes${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}wasm${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}asmjs${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}emscripten-optimizer${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ir${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cfg${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}support${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

if(MSVC)
  # Overwrite build and install commands to force Release build on MSVC.
  set(build_command BUILD_COMMAND cmake --build <BINARY_DIR> --config Release)
  set(install_command INSTALL_COMMAND cmake --build <BINARY_DIR> --config Release --target install)
elseif(CMAKE_GENERATOR STREQUAL Ninja)
  # For Ninja we have to pass the number of jobs from CI environment.
  # Otherwise it will go crazy and run out of memory.
  if($ENV{BUILD_PARALLEL_JOBS})
    set(build_command BUILD_COMMAND cmake --build <BINARY_DIR> --target binaryen -- -j $ENV{BUILD_PARALLEL_JOBS})
    message(STATUS "Ninja $ENV{BUILD_PARALLEL_JOBS}")
  endif()
endif()


ExternalProject_Add(binaryen
  PREFIX
    ${prefix}
  DOWNLOAD_NAME
    1.38.28.tar.gz
  DOWNLOAD_DIR
    ${prefix}/downloads
  SOURCE_DIR
    ${source_dir}
  BINARY_DIR
    ${binary_dir}
  URL
    https://github.com/WebAssembly/binaryen/archive/1.38.28.tar.gz
  URL_HASH
    SHA1=0bf5857641924c8f504a18da4295b5eef5c39369
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_STATIC_LIB=ON
    -DENABLE_WERROR=OFF
  ${build_command}
  ${install_command}
  TEST_COMMAND    "" # remove test step
  UPDATE_COMMAND  "" # remove update step
  BUILD_BYPRODUCTS
    ${binaryen_library}
    ${binaryen_other_libraries}
  )

add_library(binaryen::binaryen STATIC IMPORTED)

file(MAKE_DIRECTORY ${binaryen_include_dir})  # Must exist.
set_target_properties(
  binaryen::binaryen
  PROPERTIES
  IMPORTED_CONFIGURATIONS Release
  IMPORTED_LOCATION_RELEASE ${binaryen_library}
  INTERFACE_INCLUDE_DIRECTORIES ${binaryen_include_dir}
  INTERFACE_LINK_LIBRARIES "${binaryen_other_libraries}"

)

add_dependencies(binaryen::binaryen binaryen)
