#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(telemetry_message_pool_test
    message_pool_test.cpp
    )
target_link_libraries(telemetry_message_pool_test
    telemetry
    )
