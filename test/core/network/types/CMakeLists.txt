#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_request_test
    blocks_request_test.cpp
    )
target_link_libraries(block_request_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(uvar_adapter_test
        uvar_adapter_test.cpp
        )

addtest(proto_block_response_test
        protobuf_block_response_test.cpp
        )
target_link_libraries(proto_block_response_test
        node_api_proto
        blob
        scale
        )

addtest(proto_block_request_test
        protobuf_block_request_test.cpp
        )
target_link_libraries(proto_block_request_test
        node_api_proto
        blob
        )

addtest(block_response_test
    blocks_response_test.cpp
    )
target_link_libraries(block_response_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_announce_test
    block_announce_test.cpp
    )
target_link_libraries(block_announce_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_attributes_test
    block_attributes_test.cpp
    )
target_link_libraries(block_attributes_test
    scale
    )

addtest(block_direction_test
    block_direction_test.cpp
    )
target_link_libraries(block_direction_test
    scale
    )
