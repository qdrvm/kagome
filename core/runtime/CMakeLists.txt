#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(memory
    common.hpp
    wasm_memory.hpp
    impl/wasm_memory_impl.hpp
    impl/wasm_memory_impl.cpp
    )
target_link_libraries(memory
    buffer
)
