#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(bip39_integration_test
    bip39_integration_test.cpp
    )
target_link_libraries(bip39_integration_test
    bip39_provider
    logger_for_tests
    )

addtest(entropy_calculation_test
    entropy_calculation_test.cpp
    )
target_link_libraries(entropy_calculation_test
    bip39_provider
    logger_for_tests
    )
