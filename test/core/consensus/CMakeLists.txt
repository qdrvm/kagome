#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(grandpa)
add_subdirectory(validation)
add_subdirectory(babe)

addtest(babe_lottery_test
    babe_lottery_test.cpp
    )
target_link_libraries(babe_lottery_test
    babe_lottery
    )
