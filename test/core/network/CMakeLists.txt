#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(peer_client_test
    peer_client_test.cpp
    )
target_link_libraries(peer_client_test
    peer_client
    testutil_read_writer_helper
    )

addtest(peer_server_test
    peer_server_test.cpp
    )
target_link_libraries(peer_server_test
    peer_server
    testutil_read_writer_helper
    )
