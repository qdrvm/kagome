#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

function(external_project_link_libraries target prefix)
    set(targets
        app_config
        executor
        binaryen_module_factory
        binaryen_runtime_external_interface
        binaryen_wasm_memory
        binaryen_wasm_memory_factory
        binaryen_wasm_module
        binaryen_memory_provider
        core_api_factory
        core_api
        storage
        trie_storage_provider
        log_configurator
        host_api_factory
        chain_spec
        key_store
        sr25519_provider
        ed25519_provider
        pbkdf2_provider
        hasher
        storage
        module_repository
        storage_code_provider
        offchain_persistent_storage
        offchain_worker_pool
        blockchain
        runtime_upgrade_tracker
        runtime_properties_cache
        )
    list(TRANSFORM targets PREPEND "${prefix}")
    target_link_libraries(${target} ${targets})
endfunction()

