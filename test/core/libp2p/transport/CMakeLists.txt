# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(libp2p_tcp_transport_integrational_test tcp_transport_integrational_test.cpp)
target_link_libraries(libp2p_tcp_transport_integrational_test
  libp2p_tcp_transport
  )

addtest(libp2p_tcp_edge_cases tcp_edge_cases.cpp)
target_link_libraries(libp2p_tcp_edge_cases
  libp2p_tcp_transport
  )
