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

add_library(tagged_transaction_queue
    tagged_transaction_queue.hpp
    impl/tagged_transaction_queue_impl.cpp
    impl/tagged_transaction_queue_impl.hpp
    )
target_link_libraries(tagged_transaction_queue
    buffer
    logger
    scale_codec
    wasm_memory_stream
    wasm_executor
    )

add_library(parachain_host_api
    parachain_host.hpp
    impl/parachain_host_impl.hpp
    impl/parachain_host_impl.cpp
    )
target_link_libraries(parachain_host_api
    buffer
    logger
    scale_codec
    wasm_memory_stream
    wasm_executor
    )

add_library(grandpa_api
    grandpa.hpp
    impl/grandpa_impl.hpp
    impl/grandpa_impl.cpp
    )
target_link_libraries(grandpa_api
    buffer
    logger
    scale_codec
    wasm_memory_stream
    wasm_executor
    )
