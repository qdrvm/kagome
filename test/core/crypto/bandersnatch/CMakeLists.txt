#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(bandersnatch_test
    bandersnatch_test.cpp
    )
target_link_libraries(bandersnatch_test
    bip39_provider
    bandersnatch_provider
    bandersnatch_vrf
    p2p::p2p_random_generator
    logger_for_tests
    )
