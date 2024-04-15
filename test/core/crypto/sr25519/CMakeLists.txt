#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(sr25519_provider_test
    sr25519_provider_test.cpp
    )
target_link_libraries(sr25519_provider_test
    logger_for_tests
    )
