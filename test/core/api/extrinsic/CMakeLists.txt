#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(extrinsic_service_test
    extrinsic_service_test.cpp
    )
target_link_libraries(extrinsic_service_test
    extrinsic_api_service
    api_jrpc_server
    api_transport
    blob
    )

addtest(extrinsic_api_test
    extrinsic_api_test.cpp
    )
target_link_libraries(extrinsic_api_test
    extrinsic_api_service
    blob
    hasher
    testutil_primitives_generator
    )

addtest(extrinsic_api_http_integration_test
    extrinsic_api_http_integration_test.cpp
    )
target_link_libraries(extrinsic_api_http_integration_test
    extrinsic_api_service
    api_transport
    api_jrpc_server
    test_http_client
    )
