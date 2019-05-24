add_library(sr25519 UNKNOWN IMPORTED)

set(URL https://github.com/Warchant/sr25519-crust)
set(VERSION 4f430f55b7418a14de116cf173f972d1b23e0181)

externalproject_add(warchant_sr25519_crust
    GIT_REPOSITORY ${URL}
    GIT_TAG        ${VERSION}
    CMAKE_ARGS
      -DTESTING=OFF
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DBUILD_SHARED_LIBS=FALSE
    PATCH_COMMAND   ""
    INSTALL_COMMAND "" # remove install step
    TEST_COMMAND    "" # remove test step
    UPDATE_COMMAND  "" # remove update step
    )

externalproject_get_property(warchant_sr25519_crust binary_dir)
externalproject_get_property(warchant_sr25519_crust source_dir)
set(sr25519_INCLUDE_DIR ${source_dir}/include)
set(sr25519_LIBRARY ${binary_dir}/${CMAKE_STATIC_LIBRARY_PREFIX}sr25519${CMAKE_STATIC_LIBRARY_SUFFIX})
file(MAKE_DIRECTORY ${sr25519_INCLUDE_DIR})
link_directories(${binary_dir})

add_dependencies(sr25519 warchant_sr25519_crust)

set_target_properties(sr25519 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${sr25519_INCLUDE_DIR}
    IMPORTED_LOCATION ${sr25519_LIBRARY}
    )
