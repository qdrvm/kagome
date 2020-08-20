#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(http_listener_test
    http_listener_test.cpp
    )
target_link_libraries(http_listener_test
    test_http_client
    api_jrpc_server
    api_transport
    api_service
    buffer
    )

addtest(ws_listener_test
    ws_listener_test.cpp
    )
target_link_libraries(ws_listener_test
    test_ws_client
    api_jrpc_server
    api_transport
    api_service
    buffer
    )
