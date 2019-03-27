#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_info_test
    peer_info_test.cpp
    )
target_link_libraries(peer_info_test
    peer
    )
