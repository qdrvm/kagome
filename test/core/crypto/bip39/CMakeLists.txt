#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(bip39_integration_test
    bip39_integration_test.cpp
    )
target_link_libraries(bip39_integration_test
    bip39_provider
    hexutil
    )
