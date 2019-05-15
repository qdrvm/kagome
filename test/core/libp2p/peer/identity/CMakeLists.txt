#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_identity_factory_test
    peer_identity_factory_test.cpp
    )
target_link_libraries(peer_identity_factory_test
    libp2p_peer_identity
    )
