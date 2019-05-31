# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(wasm_test
    wasm_test.hpp
    wasm_test.cpp
    )
target_link_libraries(
    wasm_test
    Boost::filesystem
    buffer
    wasm_memory_stream
    wasm_executor
)
