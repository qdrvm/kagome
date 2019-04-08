# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_transport_tcp_connection
  tcp_connection.cpp
  )
target_link_libraries(libp2p_transport_tcp_connection
  Boost::boost
  buffer
  multiaddress
  )

add_library(libp2p_transport_tcp_server
  tcp_server.cpp
  )
target_link_libraries(libp2p_transport_tcp_server
  Boost::boost
  )

