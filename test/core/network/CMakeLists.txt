#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(types)

addtest(rpc_libp2p_test
    rpc_libp2p_test.cpp
    )
target_link_libraries(rpc_libp2p_test
    scale_message_read_writer
    scale
    outcome
    blob
    testutil_read_writer_helper
    p2p::p2p_peer_id
    p2p::p2p_multiaddress
    )

addtest(sync_protocol_observer_test
    sync_protocol_observer_test.cpp
    )
target_link_libraries(sync_protocol_observer_test
    sync_protocol_observer
    polkadot_trie
    )

# TODO(xDimon): would be good to make test for sync_protocol_client
