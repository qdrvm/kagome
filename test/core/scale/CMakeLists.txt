#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(big_fixed_integers_test
        big_fixed_integers_test.cpp
        )
target_link_libraries(big_fixed_integers_test
        scale::scale
        hexutil
        )
