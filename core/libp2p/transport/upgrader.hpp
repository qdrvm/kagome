/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UPGRADER_HPP
#define KAGOME_UPGRADER_HPP

#include <memory>

#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/connection/raw_connection.hpp"
#include "libp2p/connection/secure_connection.hpp"
#include "libp2p/connection/stream.hpp"

namespace libp2p::transport {

  // upgrader knows about all transport and muxer adaptors, knows current peer
  // id it uses multiselect to negotiate all protocols in 2 rounds:
  // 1. upgrade security by finding intersection of our supported adaptors and
  // supported adaptors of remote peer
  // 2. upgrade muxer by finding intersection of our supported adaptors and
  // supported adaptors of remote peer
  struct Upgrader {
    virtual ~Upgrader() = default;

    // upgrade raw connection to secure connection
    virtual std::shared_ptr<connection::SecureConnection> upgradeToSecure(
        std::shared_ptr<connection::RawConnection> conn) = 0;

    // upgrade secure connection to capable connection
    virtual std::shared_ptr<connection::CapableConnection> upgradeToMuxed(
        std::shared_ptr<connection::SecureConnection> conn,
        std::function<connection::Stream::Handler> f) = 0;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_UPGRADER_HPP
