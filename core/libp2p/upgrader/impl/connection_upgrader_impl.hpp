/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CONNECTION_UPGRADER_CONNECTION_UPGRADER_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CONNECTION_UPGRADER_CONNECTION_UPGRADER_IMPL_HPP

#include "libp2p/upgrader/connection_upgrader.hpp"

namespace libp2p::upgrader {
  /**
   * @class ConnectionUpgrader connection upgrader
   */
  class ConnectionUpgraderImpl : public ConnectionUpgrader {
   public:
    using NewStreamHandler = muxer::Yamux::NewStreamHandler;

    outcome::result<std::unique_ptr<transport::MuxedConnection>> upgradeToMuxed(
        std::shared_ptr<transport::Connection> connection,
        MuxerOptions muxer_options, NewStreamHandler handler) const override;
  };
}  // namespace libp2p::upgrader

#endif  // KAGOME_CORE_LIBP2P_CONNECTION_UPGRADER_CONNECTION_UPGRADER_IMPL_HPP
