#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(common)
add_subdirectory(binaryen)

add_library(wasm_compiler INTERFACE)

if ("${WASM_COMPILER}" STREQUAL "WAVM")
  add_subdirectory(wavm)
  target_link_libraries(wasm_compiler INTERFACE runtime_wavm)
  target_compile_definitions(wasm_compiler INTERFACE
      KAGOME_WASM_COMPILER_WAVM=1
      KAGOME_WASM_COMPILER_WASM_EDGE=0
      )
elseif ("${WASM_COMPILER}" STREQUAL "WasmEdge")
  add_subdirectory(wasm_edge)
  target_link_libraries(wasm_compiler INTERFACE runtime_wasm_edge)
  target_compile_definitions(wasm_compiler INTERFACE
      KAGOME_WASM_COMPILER_WAVM=0
      KAGOME_WASM_COMPILER_WASM_EDGE=1
  )
else()
  fatal_error("Unknown WASM_COMPILER: ${WASM_COMPILER}")
endif()

add_subdirectory(runtime_api/impl)
