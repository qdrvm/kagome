#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_id_test
    peer_id_test.cpp
    )
target_link_libraries(peer_id_test
    libp2p-peer
    )

addtest(peer_id_manager_test
    peer_id_manager_test.cpp
    )
target_link_libraries(peer_id_manager_test
    libp2p-peer
    )
