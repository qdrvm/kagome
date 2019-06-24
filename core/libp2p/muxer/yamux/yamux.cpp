/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include "libp2p/connection/yamux/yamuxed_connection.hpp"

namespace libp2p::muxer {
  Yamux::Yamux(StreamHandlerFunc handler, MuxedConnectionConfig config)
      : handler_{std::move(handler)}, config_{config} {}

  peer::Protocol Yamux::getProtocolId() const noexcept {
    return "/yamux/1.0.0";
  }

  outcome::result<std::shared_ptr<connection::CapableConnection>>
  Yamux::muxConnection(
      std::shared_ptr<connection::SecureConnection> conn) const {
    return std::make_shared<connection::YamuxedConnection>(std::move(conn),
                                                           handler_, config_);
  }
}  // namespace libp2p::muxer
