#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(extrinsic_service_test
    extrinsic_service_test.cpp
    )
target_link_libraries(extrinsic_service_test
    extrinsic_api_service
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

add_library(test_tcp_client
    simple_client.hpp
    simple_client.cpp
    )
target_link_libraries(test_tcp_client
    Boost::boost
    )

addtest(extrinsic_api_integration_test
    extrinsic_api_integration_test.cpp
    )
target_link_libraries(extrinsic_api_integration_test
    extrinsic_api_service
    api_transport
    test_tcp_client
    )
