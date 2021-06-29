#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_api_factory
    host_api_factory_impl.cpp
    )
target_link_libraries(host_api_factory
    buffer
    host_api
    wavm_executor # TODO(Harrm): might not actually need it, need to remove WAVM header leakage
    )

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    bip39_provider
    hasher
    logger
    p2p::p2p_random_generator
    sr25519_provider
    secp256k1_provider
    ed25519_provider
    scale
    crypto_store
    wavm_memory
    )
kagome_install(crypto_extension)

add_library(io_extension
    io_extension.cpp
    )
target_link_libraries(io_extension
    hexutil
    logger
    buffer
    wavm_memory
    )
kagome_install(io_extension)

add_library(memory_extension
    memory_extension.cpp
    )
target_link_libraries(memory_extension
    logger
    buffer
    wavm_memory
    )
kagome_install(memory_extension)

add_library(misc_extension
    misc_extension.cpp
    )
target_link_libraries(misc_extension
    scale
    logger
    constant_code_provider
    outcome
    blob
    wavm_memory
    wavm_executor
    )
kagome_install(misc_extension)

add_library(storage_extension
    storage_extension.cpp
    )
target_link_libraries(storage_extension
    buffer
    blob
    logger
    ordered_trie_hash
    scale_encode_append
    runtime_transaction_error
    wavm_memory
    )
kagome_install(storage_extension)
