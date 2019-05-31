#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(runtime_external_interface
    runtime_external_interface.hpp
    runtime_external_interface.cpp
    )

target_link_libraries(runtime_external_interface
    binaryen::binaryen
    extensions
    )

add_library(wasm_executor
    wasm_executor.hpp
    wasm_executor.cpp
    )
target_link_libraries(wasm_executor
    binaryen::binaryen
    extensions
    runtime_external_interface
    )
