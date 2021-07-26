#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_wasm_provider
    storage_wasm_provider.cpp
    uncompress_code_if_needed.cpp
    )
target_link_libraries(storage_wasm_provider
    blob
    buffer
    zstd::libzstd_static
    )
kagome_install(storage_wasm_provider)

add_library(const_wasm_provider
    const_wasm_provider.cpp
    )
target_link_libraries(const_wasm_provider
    buffer
    )

kagome_install(const_wasm_provider)

add_library(runtime_transaction_error
    runtime_transaction_error.cpp
    )
target_link_libraries(runtime_transaction_error
    outcome
    )
kagome_install(runtime_transaction_error)

add_library(trie_storage_provider
    trie_storage_provider_impl.cpp
    )
target_link_libraries(trie_storage_provider
    runtime_transaction_error
    trie_storage
    blob
    )
kagome_install(trie_storage_provider)
