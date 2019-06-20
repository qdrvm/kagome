#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(yamux_test
    yamux_integration_test.cpp
    )
target_link_libraries(yamux_test
    libp2p_yamux
    libp2p_yamuxed_connection
    multiaddress
    transport_test_fixture
    security_plaintext
    )

addtest(yamux_frame_test
    yamux_frame_test.cpp
    )
target_link_libraries(yamux_frame_test
    libp2p_yamuxed_connection
    )
