#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(sr25519_provider_test
    sr25519_provider_test.cpp
    )
target_link_libraries(sr25519_provider_test
    bip39_provider
    logger_for_tests
    sr25519_provider
    )
