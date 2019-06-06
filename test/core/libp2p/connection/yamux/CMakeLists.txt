#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(yamux_test
    yamuxed_connection_integration_test.cpp
    )
target_link_libraries(yamux_test
    yamux
    multiaddress
    transport_test_fixture
    libp2p_peer_id
    security_plaintext
    )

addtest(yamux_frame_test
    yamux_frame_test.cpp
    )
target_link_libraries(yamux_frame_test
    yamux
    )
