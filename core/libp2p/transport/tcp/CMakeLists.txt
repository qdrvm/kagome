# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

# transport -> listener -> connection

add_library(libp2p_tcp_connection
  tcp_connection.cpp
  )
target_link_libraries(libp2p_tcp_connection
  Boost::boost
  multiaddress
  )

add_library(libp2p_tcp_listener
  tcp_listener.cpp
  )
target_link_libraries(libp2p_tcp_listener
  Boost::boost
  libp2p_tcp_connection
  multiaddress
  libp2p_transport_base_listener
  )


add_library(libp2p_tcp_transport
  tcp_transport.cpp
  )
target_link_libraries(libp2p_tcp_transport
  Boost::boost
  libp2p_tcp_listener
  multiaddress
  )



