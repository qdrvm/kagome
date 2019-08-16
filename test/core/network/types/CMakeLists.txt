#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_request_test
    block_request_test.cpp
    )
target_link_libraries(block_request_test
    buffer
    scale
    testutil_primitives_generator
    )

addtest(block_response_test
    block_response_test.cpp
    )
target_link_libraries(block_request_test
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
