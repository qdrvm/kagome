#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(plaintext_connection_test
    plaintext_connection_test.cpp
    )
target_link_libraries(plaintext_connection_test
    security_plaintext
    multiaddress
    )
