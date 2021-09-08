#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(wasmedge_wasm_memory
    memory_factory.cpp
    memory_impl.cpp
    memory_provider.cpp
    )
target_link_libraries(wasmedge_wasm_memory
    buffer
    logger
    CONAN_PKG::WasmEdge
    memory_allocator
    )
kagome_install(wasmedge_wasm_memory)

add_library(wasmedge_wasm_runtime
  module_instance_impl.cpp)
target_link_libraries(wasmedge_wasm_runtime
  wasmedge_wasm_memory)
kagome_install(wasmedge_wasm_runtime)
