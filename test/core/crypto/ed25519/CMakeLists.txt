#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(ed25519_provider_test
    ed25519_provider_test.cpp
    )
target_link_libraries(ed25519_provider_test
    logger_for_tests
    )
