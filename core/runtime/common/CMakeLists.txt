#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage_code_provider
    storage_code_provider.cpp
    )
target_link_libraries(storage_code_provider
    blob
    logger
    uncompress_if_needed
    )
kagome_install(storage_code_provider)

add_library(constant_code_provider
    constant_code_provider.cpp
    )
target_link_libraries(constant_code_provider
    uncompress_if_needed
    )
kagome_install(constant_code_provider)

add_library(uncompress_if_needed
    uncompress_code_if_needed.cpp
    )
target_link_libraries(uncompress_if_needed
    buffer
    zstd::libzstd_static
    )
kagome_install(uncompress_if_needed)

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
    trie_serializer
    blob
    )
kagome_install(trie_storage_provider)

add_library(runtime_upgrade_tracker runtime_upgrade_tracker_impl.cpp)
target_link_libraries(runtime_upgrade_tracker
    logger
    )
kagome_install(runtime_upgrade_tracker)

add_library(module_repository module_repository_impl.cpp)
target_link_libraries(module_repository outcome)
kagome_install(module_repository)

add_library(runtime_environment_factory runtime_environment_factory.cpp)
target_link_libraries(runtime_environment_factory
    logger
    trie_error
    mp_utils
    )
kagome_install(runtime_environment_factory)

add_library(memory_allocator memory_allocator.cpp)
target_link_libraries(memory_allocator Boost::boost)
kagome_install(memory_allocator)
