#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(author_service_test
    author_service_test.cpp
    )
target_link_libraries(author_service_test
    author_api_service
    api_jrpc_server
    api_transport
    blob
    )

addtest(author_api_test
    author_api_test.cpp
    )
target_link_libraries(author_api_test
    author_api_service
    blob
    hasher
    testutil_primitives_generator
    )

addtest(author_api_http_integration_test
    author_api_http_integration_test.cpp
    )
target_link_libraries(author_api_http_integration_test
    author_api_service
    api_transport
    api_jrpc_server
    test_http_client
    )
