#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(application_injector INTERFACE)
target_link_libraries(application_injector INTERFACE
    p2p::p2p
    tsl::tsl_hat_trie
    Boost::Boost.DI
    api_transport
    babe
    babe_lottery
    babe_observer
    block_header_repository
    block_storage
    block_tree
    buffer
    clock
    configuration_storage
    core
    extrinsic_api_service
    storage_wasm_provider
    gossiper_broadcast
    in_memory_storage
    kagome_router
    leveldb
    outcome
    proposer
    synchronizer
    tagged_transaction_queue
    vrf_provider
    waitable_timer
    wasm_executor
    )
