#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(address_publisher_test
    address_publisher_test.cpp
    )
target_link_libraries(address_publisher_test
    address_publisher
    crypto_store
    logger_for_tests
    )
