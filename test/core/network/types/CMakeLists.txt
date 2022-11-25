#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(uvar_adapter_test
    uvar_adapter_test.cpp
    )
target_link_libraries(uvar_adapter_test
    network
    )

addtest(message_read_writer_test
    message_read_writer_test.cpp
    )

addtest(proto_block_response_test
    protobuf_block_response_test.cpp
    )
target_link_libraries(proto_block_response_test
    node_api_proto
    blob
    scale::scale
    network
    )

addtest(proto_block_request_test
    protobuf_block_request_test.cpp
    )
target_link_libraries(proto_block_request_test
    node_api_proto
    blob
    network
    p2p::p2p_uvarint
    )

addtest(proto_state_response_test
    protobuf_state_response_test.cpp
    )
target_link_libraries(proto_state_response_test
    network
    blob
    node_api_proto
    )

addtest(proto_state_request_test
    protobuf_state_request_test.cpp
    )
target_link_libraries(proto_state_request_test
    node_api_proto
    blob
    network
    p2p::p2p_uvarint
    )

addtest(block_announce_test
    block_announce_test.cpp
    )
target_link_libraries(block_announce_test
    scale::scale
    testutil_primitives_generator
    )

addtest(block_attributes_test
    block_attributes_test.cpp
    )
target_link_libraries(block_attributes_test
    scale::scale
    )

addtest(block_direction_test
    block_direction_test.cpp
    )
target_link_libraries(block_direction_test
    scale::scale
    )

addtest(bootstrap_nodes_test
    bootstrap_nodes_test.cpp
)
target_link_libraries(bootstrap_nodes_test
    p2p::p2p
    blob
)
