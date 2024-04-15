#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(ecdsa_provider_test
    ecdsa_provider_test.cpp
    )
target_link_libraries(ecdsa_provider_test
    logger_for_tests
    )
