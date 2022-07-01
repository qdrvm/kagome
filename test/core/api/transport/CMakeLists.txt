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
    blob
    state_api_service
    logger_for_tests
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
    blob
    state_api_service
    logger_for_tests
    )

addtest(jrpc_handle_batch_test
    jrpc_handle_batch_test.cpp
    )
target_link_libraries(jrpc_handle_batch_test
    api_jrpc_server
    )
