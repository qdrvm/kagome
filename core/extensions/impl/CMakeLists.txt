#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    hasher
    p2p::p2p_random_generator
    sr25519_provider
    secp256k1_provider
    ed25519_provider
    logger
    )
kagome_install(crypto_extension)

add_library(io_extension
    io_extension.cpp
    )
target_link_libraries(io_extension
    hexutil
    logger
    buffer
    )
kagome_install(io_extension)

add_library(memory_extension
    memory_extension.cpp
    )
target_link_libraries(memory_extension
    logger
    buffer
)
kagome_install(memory_extension)

add_library(misc_extension
    misc_extension.cpp
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
    )
kagome_install(storage_extension)
