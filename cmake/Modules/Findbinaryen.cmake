set(prefix ${CMAKE_BINARY_DIR}/deps)
set(source_dir ${prefix}/src/binaryen)
set(binary_dir ${prefix}/src/binaryen-build)
# Use source dir because binaryen only installs single header with C API.
set(binaryen_include_dir ${source_dir}/src)
set(binaryen_library ${prefix}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}binaryen${CMAKE_STATIC_LIBRARY_SUFFIX})
# Include also other static libs needed:
set(binaryen_other_libraries
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}wasm${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}asmjs${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}passes${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}cfg${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ir${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}emscripten-optimizer${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${binary_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}support${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

ExternalProject_Add(binaryen
    PREFIX ${prefix}
    DOWNLOAD_NAME binaryen-1.38.26.tar.gz
    DOWNLOAD_DIR ${prefix}/downloads
    SOURCE_DIR ${source_dir}
    BINARY_DIR ${binary_dir}
    URL https://github.com/WebAssembly/binaryen/archive/1.38.26.tar.gz
    URL_HASH SHA256=b32376e3b2074a4ad60da618e2b2026054922f5fa8121b71b9658d8e5de71647
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_STATIC_LIB=ON
    ${build_command}
    ${install_command}
    BUILD_BYPRODUCTS ${binaryen_library} ${binaryen_other_libraries}
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
