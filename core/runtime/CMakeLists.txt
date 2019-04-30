#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(memory
    common.hpp
    wasm_memory.hpp
    impl/wasm_memory_impl.hpp
    impl/wasm_memory_impl.cpp
    )
target_link_libraries(memory
    buffer
    )

add_library(core
    core.hpp
    impl/core_impl.hpp
    impl/core_impl.cpp
    )
target_link_libraries(core
    buffer
    logger
    scale_codec
    wasm_memory_stream
    wasm_executor
    )
