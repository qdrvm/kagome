
function(external_project_link_libraries target prefix)
    set(targets
        app_config
        executor
        runtime_wavm
        core_api
        storage
        trie_storage_provider
        log_configurator
        host_api_factory
        chain_spec
        crypto_store
        key_file_storage
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

