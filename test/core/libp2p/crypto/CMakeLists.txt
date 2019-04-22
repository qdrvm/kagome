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
    aes_provider
    crypto_error
    )

addtest(hmac_test
    hmac_test.cpp
    )
target_link_libraries(hmac_test
    crypto_provider
    )

addtest(random_test
    random_test.cpp
    )
target_link_libraries(random_test
    buffer
    random_provider
    )

addtest(marshaller_test
    marshaller_test.cpp
    )
target_link_libraries(marshaller_test
    crypto_provider
    )
