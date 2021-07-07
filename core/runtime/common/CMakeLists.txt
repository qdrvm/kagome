#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#


add_library(storage_code_provider
    storage_code_provider.cpp
    )
target_link_libraries(storage_code_provider
    buffer
    blob
    )

add_library(constant_code_provider
    constant_code_provider.cpp
    )
target_link_libraries(constant_code_provider
    buffer
    )

kagome_install(constant_code_provider)

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

add_library(runtime_upgrade_tracker runtime_upgrade_tracker_impl.cpp)
target_link_libraries(runtime_upgrade_tracker
    logger
    )
kagome_install(runtime_upgrade_tracker)
