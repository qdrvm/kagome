#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(payment_api_test
        payment_api_test.cpp
        )
target_link_libraries(payment_api_test
        payment_api_service
        blob
        )
