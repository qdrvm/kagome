#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(key_type_test
    key_type_test.cpp
    )
target_link_libraries(key_type_test
    crypto_store_key_type
    )
