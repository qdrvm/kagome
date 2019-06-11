# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(tcp_connection tcp_connection.cpp)
target_link_libraries(tcp_connection
    Boost::boost
    ufiber
    multiaddress
    )

add_library(tcp_listener tcp_listener.cpp)
target_link_libraries(tcp_listener
    tcp_connection
    )

add_library(tcp_transport tcp_transport.cpp)
target_link_libraries(tcp_transport
    tcp_connection
    tcp_listener
    )
