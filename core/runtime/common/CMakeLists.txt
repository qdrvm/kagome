#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(runtime_common
    stack_limiter.cpp
    memory_error.cpp
    )
target_link_libraries(runtime_common
    wabt::wabt
    blob
    logger
    uncompress_if_needed
    outcome
    )
kagome_install(runtime_common)

add_library(storage_code_provider
    storage_code_provider.cpp
    )
target_link_libraries(storage_code_provider
    blob
    logger
    uncompress_if_needed
    )
kagome_install(storage_code_provider)

add_library(uncompress_if_needed
    uncompress_code_if_needed.cpp
    )
target_link_libraries(uncompress_if_needed
    zstd::libzstd_static
    )
kagome_install(uncompress_if_needed)

add_library(runtime_transaction_error
    runtime_execution_error.cpp
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
    storage
    blob
    )
kagome_install(trie_storage_provider)

add_library(runtime_upgrade_tracker
    runtime_upgrade_tracker_impl.cpp
    )
target_link_libraries(runtime_upgrade_tracker
    blockchain
    logger
    )
kagome_install(runtime_upgrade_tracker)

add_library(module_repository
    module_repository_impl.cpp
    runtime_instances_pool.cpp)
target_link_libraries(module_repository
    outcome
    uncompress_if_needed
    blob
    executor
    runtime_common
    )
kagome_install(module_repository)

add_library(executor
    executor.cpp
    runtime_context.cpp
    module_instance.cpp
    )
target_link_libraries(executor
    logger
    uncompress_if_needed
    storage
    mp_utils
    runtime_common
    )
kagome_install(executor)

add_library(memory_allocator
    memory_allocator.cpp
    )
target_link_libraries(memory_allocator
    Boost::boost
    outcome
    )
kagome_install(memory_allocator)

add_library(core_api_factory core_api_factory_impl.cpp)
target_link_libraries(core_api_factory
    core_api
    outcome
    )
kagome_install(core_api_factory)
