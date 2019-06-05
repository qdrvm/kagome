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

    using RawSPtr = std::shared_ptr<connection::RawConnection>;
    using SecureSPtr = std::shared_ptr<connection::SecureConnection>;
    using CapableSPtr = std::shared_ptr<connection::CapableConnection>;

    // upgrade raw connection to secure connection
    virtual outcome::result<SecureSPtr> upgradeToSecure(RawSPtr conn) = 0;

    // upgrade secure connection to capable connection
    virtual outcome::result<CapableSPtr> upgradeToMuxed(SecureSPtr conn) = 0;
  };

}  // namespace libp2p::transport

#endif  // KAGOME_UPGRADER_HPP
