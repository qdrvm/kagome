#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(test_http_client
    http_client.cpp
    )
target_link_libraries(test_http_client
    Boost::boost
    outcome
    )

add_library(test_ws_client
    ws_client.cpp
    )
target_link_libraries(test_ws_client
    Boost::boost
    )
