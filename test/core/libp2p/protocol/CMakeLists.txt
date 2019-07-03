#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(identify_test
    identify_test.cpp
    )
target_link_libraries(identify_test
    libp2p_identify
    )
