#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(keys_test
        keys_test.cpp
        )

target_link_libraries(keys_test
        crypto_provider
        )

addtest(aes_test
        aes_test.cpp
        )

target_link_libraries(aes_test
        crypto_provider
        )

addtest(hmac_test
        hmac_test.cpp
        )

target_link_libraries(hmac_test
        crypto_provider
        )
