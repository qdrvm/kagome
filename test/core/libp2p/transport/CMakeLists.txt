# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(tcp_transport_integrational_test tcp_transport_integrational_test.cpp)
target_link_libraries(tcp_transport_integrational_test
  libp2p_tcp_transport
  )
