#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(host_api_factory
    host_api_factory_impl.cpp
    )
target_link_libraries(host_api_factory
    host_api
    )
kagome_install(host_api_factory)

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    bip39_provider
    hasher
    logger
    p2p::p2p_random_generator
    ecdsa_provider
    sr25519_provider
    secp256k1_provider
    ed25519_provider
    scale::scale
    crypto_store
    )
kagome_install(crypto_extension)

add_library(io_extension
    io_extension.cpp
    )
target_link_libraries(io_extension
    hexutil
    logger
    )
kagome_install(io_extension)

add_library(memory_extension
    memory_extension.cpp
    )
target_link_libraries(memory_extension
    logger
    )
kagome_install(memory_extension)

add_library(misc_extension
    misc_extension.cpp
    )
target_link_libraries(misc_extension
    scale::scale
    logger
    constant_code_provider
    outcome
    blob
    )
kagome_install(misc_extension)

add_library(storage_extension
    storage_extension.cpp
    )
target_link_libraries(storage_extension
    blob
    logger
    storage
    scale::scale_encode_append
    runtime_transaction_error
    )
kagome_install(storage_extension)

add_library(child_storage_extension
    child_storage_extension.cpp
    )
target_link_libraries(child_storage_extension
    blob
    logger
    storage
    scale::scale_encode_append
    runtime_transaction_error
    )
kagome_install(child_storage_extension)

add_library(offchain_extension
    offchain_extension.cpp
    )
target_link_libraries(offchain_extension
    blob
    logger
    p2p::p2p_peer_id
    p2p::p2p_multiaddress
    )
kagome_install(offchain_extension)
