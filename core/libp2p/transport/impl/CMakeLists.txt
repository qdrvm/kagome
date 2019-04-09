# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0


add_library(libp2p_transport_listener
  transport_listener_impl.cpp
  )
target_link_libraries(libp2p_transport_listener
  Boost::boost
  libp2p_transport_tcp_server
  libp2p_transport_tcp_connection
  )


add_library(libp2p_transport
  transport_impl.cpp
  )
target_link_libraries(libp2p_transport
  Boost::boost
  libp2p_transport_listener
  libp2p_transport_tcp_connection
  )
