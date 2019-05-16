#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_id_test
    peer_id_test.cpp
    )
target_link_libraries(peer_id_test
    libp2p_peer_id
    )

addtest(peer_identity_test
    peer_identity_test.cpp
    )
target_link_libraries(peer_identity_test
    libp2p_peer_identity
    )

add_subdirectory(address_repository)
add_subdirectory(key_book)
add_subdirectory(protocol_repository)
