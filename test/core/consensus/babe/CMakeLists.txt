#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(babe_test
    babe_test.cpp
    )
target_link_libraries(babe_test
    babe
    clock
    sr25519_types
    sr25519_provider
    )
