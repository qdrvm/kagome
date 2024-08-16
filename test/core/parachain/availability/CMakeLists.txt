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
    dummy_error
)
