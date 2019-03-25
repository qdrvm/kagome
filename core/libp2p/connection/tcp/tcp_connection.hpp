/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCP_CONNECTION_HPP
#define KAGOME_TCP_CONNECTION_HPP

#include "libp2p/connection/connection.hpp"

namespace libp2p::connection {

  /**
   * TCP implementation of connection interface
   */
  class TcpConnection : public connection::Connection {};

}  // namespace libp2p::connection

#endif  // KAGOME_TCP_CONNECTION_HPP
