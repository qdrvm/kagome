#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(validator_parachain
    availability/bitfield/signer.cpp
    availability/bitfield/store_impl.cpp
    availability/erasure_coding_error.cpp
    availability/fetch/fetch_impl.cpp
    availability/recovery/recovery_impl.cpp
    availability/store/store_impl.cpp
    backing/store_impl.cpp
    pvf/precheck.cpp
    pvf/pvf_impl.cpp
    pvf/module_precompiler.cpp
    validator/impl/parachain_observer_impl.cpp
    validator/impl/parachain_processor.cpp
    validator/signer.cpp
    approval/approval_distribution.cpp
    approval/approval_distribution_error.cpp
    approval/approval.cpp
    backing/store_impl.cpp
    backing/cluster.cpp
    validator/impl/fragment_tree.cpp
    validator/backing_implicit_view.cpp
    )

target_link_libraries(validator_parachain
    key_store
    dispute_coordinator
    module_repository
    network
    erasure_coding_crust::ec-cpp
    waitable_timer
    kagome_pvf_worker
    runtime_common
    )

add_library(kagome_pvf_worker
    pvf/kagome_pvf_worker.cpp
    )
target_link_libraries(kagome_pvf_worker
    fmt::fmt
    scale::scale
    soralog::fallback_configurator
    soralog::soralog
    logger
    log_configurator

    binaryen_wasm_memory
    binaryen_wasm_memory_factory
    binaryen_runtime_external_interface
    binaryen_memory_provider
    binaryen_wasm_module
    binaryen_instance_environment_factory
    binaryen_module_factory
    wasm_compiler

    runtime_properties_cache
    sr25519_provider
    ed25519_provider
    ecdsa_provider
    secp256k1_provider
    host_api_factory
    p2p::p2p_basic_scheduler
    p2p::p2p_asio_scheduler_backend
    )

