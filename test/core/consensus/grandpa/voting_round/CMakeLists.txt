#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(voting_round_test
    voting_round_test.cpp
    )

target_link_libraries(voting_round_test
    voting_round
    )
