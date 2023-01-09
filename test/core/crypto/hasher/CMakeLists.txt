#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(hasher_test
        hasher_test.cpp
        )

target_link_libraries(hasher_test
        hasher
        blob
        )
