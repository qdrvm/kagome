#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(crypto_extension
    crypto_extension.cpp
    )
target_link_libraries(crypto_extension
    iroha::ed25519
    blake2
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

add_library(sandboxing_extension
    sandboxing_extension.cpp
    )

add_library(storage_extension
    storage_extension.cpp
    )
target_link_libraries(storage_extension
    buffer
    blob
    logger
    )
