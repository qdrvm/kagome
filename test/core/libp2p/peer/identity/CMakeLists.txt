#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_identity_test
    peer_identity_test.cpp
    )
target_link_libraries(peer_identity_test
    libp2p_peer_identity
    )
