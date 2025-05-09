#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(validator/prospective_parachains)

add_library(grid_tracker
    backing/grid_tracker.cpp
    )

target_link_libraries(grid_tracker
    fmt::fmt
    scale::scale
    soralog::soralog
    logger
    Boost::boost
    outcome
    )

add_library(backing_implicit_view
    validator/backing_implicit_view.cpp
    )

target_link_libraries(backing_implicit_view
    fmt::fmt
    scale::scale
    soralog::soralog
    logger
    Boost::boost
    outcome
    )

add_library(parachain_errors
    validator/i_parachain_processor.cpp
    )

target_link_libraries(parachain_errors
    fmt::fmt
    soralog::soralog
    logger
    Boost::boost
    outcome
    )

add_library(validator_parachain
    availability/bitfield/signer.cpp
    availability/bitfield/store_impl.cpp
    availability/erasure_coding_error.cpp
    availability/fetch/fetch_impl.cpp
    availability/recovery/recovery_impl.cpp
    availability/store/store_impl.cpp
    backing/store_impl.cpp
    pvf/pool.cpp
    pvf/precheck.cpp
    pvf/pvf_impl.cpp
    pvf/module_precompiler.cpp
    pvf/workers.cpp
    validator/impl/parachain_observer_impl.cpp
    validator/impl/parachain_processor.cpp
    validator/signer.cpp
    approval/approval_distribution.cpp
    approval/approval_distribution_error.cpp
    approval/approval.cpp
    backing/store_impl.cpp
    backing/cluster.cpp
    validator/statement_distribution/statement_distribution.cpp
    validator/parachain_storage.cpp
    )

target_link_libraries(validator_parachain
    Boost::process
    grid_tracker
    key_store
    dispute_coordinator
    module_repository
    network
    erasure_coding_crust::ec-cpp
    waitable_timer
    kagome_pvf_worker
    runtime_common
    prospective_parachains
    backing_implicit_view
    parachain_errors
    )

add_library(kagome_pvf_worker
    pvf/kagome_pvf_worker.cpp
    pvf/secure_mode_precheck.cpp
    )
target_link_libraries(kagome_pvf_worker
    PUBLIC
    Boost::process
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
    bandersnatch_provider
    host_api_factory
    p2p::p2p_basic_scheduler
    p2p::p2p_asio_scheduler_backend
    )

if("${WASM_COMPILER}" STREQUAL "WasmEdge")
    target_link_libraries(kagome_pvf_worker PUBLIC
        spdlog::spdlog
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries(kagome_pvf_worker PRIVATE seccomp)
endif()
