#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    blake2
    iroha::ed25519
    random_generator
    sr25519_provider
    sr25519_types
    twox
    )

add_library(io_extension
    io_extension.cpp
    )
target_link_libraries(io_extension
    hexutil
    logger
    buffer
    )

add_library(memory_extension
    memory_extension.cpp
    )
target_link_libraries(memory_extension
    memory
    logger
)

add_library(misc_extension
    misc_extension.cpp
    )

add_library(storage_extension
    storage_extension.cpp
    )
target_link_libraries(storage_extension
    buffer
    blob
    logger
    ordered_trie_hash
    )
