/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

namespace libp2p::muxer {
  peer::Protocol Yamux::getProtocolId() const {
    return "/yamux/1.0.0";
  }

  outcome::result<std::shared_ptr<connection::CapableConnection>>
  Yamux::muxConnection(
      std::shared_ptr<connection::SecureConnection> conn) const {
    return connection::YamuxedConnection{std::move(conn)};
  }
}  // namespace libp2p::muxer
