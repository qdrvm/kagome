#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(secp256k1_provider_test
    secp256k1_provider_test.cpp
    )
target_link_libraries(secp256k1_provider_test
    secp256k1_provider
    )
