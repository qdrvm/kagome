#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(types)

addtest(block_response_cache_test
    block_response_cache_test.cpp
    )
target_link_libraries(block_response_cache_test
    p2p::p2p_peer_id
    network
    trie_storage_provider
    )

addtest(rpc_libp2p_test
    rpc_libp2p_test.cpp
    )
target_link_libraries(rpc_libp2p_test
    network
    scale::scale
    outcome
    blob
    testutil_read_writer_helper
    p2p::p2p_peer_id
    p2p::p2p_multiaddress
    logger_for_tests
    )

addtest(state_protocol_observer_test
    state_protocol_observer_test.cpp
    )
target_link_libraries(state_protocol_observer_test
    logger_for_tests
    storage
    network
    )

addtest(sync_protocol_observer_test
    sync_protocol_observer_test.cpp
    )
target_link_libraries(sync_protocol_observer_test
    network
    storage
    logger_for_tests
    )

addtest(synchronizer_test
    synchronizer_test.cpp
    )
target_link_libraries(synchronizer_test
    logger_for_tests
    network
    blockchain
    storage
    p2p::p2p_basic_scheduler
    p2p::p2p_message_read_writer
    p2p::p2p_peer_id
    p2p::p2p_literals
    )
