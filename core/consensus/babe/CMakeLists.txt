#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(babe
    impl/babe_digests_util.cpp
    impl/babe_impl.cpp
    impl/threshold_util.cpp
    impl/babe_lottery_impl.cpp
    impl/babe_config_repository_impl.cpp
    babe_error.cpp
    )
target_link_libraries(babe
#    mp_utils
#    ed25519_types
#    Boost::boost
#    schnorrkel_crust::schnorrkel_crust
#    transaction_pool_error
#    hasher
#    vrf_provider
    logger
#    scale::scale
#    primitives
#    blob
#    outcome
#    p2p::p2p_peer_id
#    storage
    metrics
#    blockchain
#    telemetry
    )
kagome_install(babe)
