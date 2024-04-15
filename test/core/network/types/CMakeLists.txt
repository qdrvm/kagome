#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(uvar_adapter_test
    uvar_adapter_test.cpp
    )

addtest(message_read_writer_test
    message_read_writer_test.cpp
    )

addtest(proto_block_response_test
    protobuf_block_response_test.cpp
    )

addtest(proto_block_request_test
    protobuf_block_request_test.cpp
    )

addtest(proto_state_response_test
    protobuf_state_response_test.cpp
    )

addtest(proto_state_request_test
    protobuf_state_request_test.cpp
    )

addtest(block_announce_test
    block_announce_test.cpp
    )
target_link_libraries(block_announce_test
    testutil_primitives_generator
    )

addtest(block_attributes_test
    block_attributes_test.cpp
    )

addtest(block_direction_test
    block_direction_test.cpp
    )

addtest(bootstrap_nodes_test
    bootstrap_nodes_test.cpp
)
