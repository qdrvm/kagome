#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(recovery_test
    recovery_test.cpp
)

target_link_libraries(recovery_test
    validator_parachain
#    log_configurator
#    base_fs_test
#    key_store
#    logger
    dummy_error
#    p2p::p2p_random_generator
#    Boost::boost
)
