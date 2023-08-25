#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(timeline)
add_subdirectory(validation)
add_subdirectory(babe)
add_subdirectory(grandpa)

addtest(babe_lottery_test
    babe_lottery_test.cpp
    )
target_link_libraries(babe_lottery_test
    consensus
    logger_for_tests
    )
