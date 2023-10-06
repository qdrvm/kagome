#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_validator_test
    block_validator_test.cpp
    )
target_link_libraries(block_validator_test
    consensus
    p2p::p2p_peer_id
    testutil_primitives_generator
    sr25519_provider
    mp_utils
    keccak
    logger_for_tests
    )
