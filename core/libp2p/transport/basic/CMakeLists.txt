# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(libp2p_transport_base_listener
  base_listener.cpp
  )
target_link_libraries(libp2p_transport_base_listener
  Boost::boost
  )
