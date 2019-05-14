/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/muxer/yamux/yamux.hpp"

#include "libp2p/muxer/yamux/yamuxed_connection.hpp"

namespace libp2p::muxer {
  Yamux::Yamux(YamuxConfig config) : config_{config} {}

  std::unique_ptr<transport::MuxedConnection> Yamux::upgrade(
      std::shared_ptr<transport::Connection> connection,
      NewStreamHandler stream_handler) const {
    return std::make_unique<YamuxedConnection>(
        std::move(connection), std::move(stream_handler), config_);
  }
}  // namespace libp2p::muxer
