/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_UPGRADER_CONNECTION_UPGRADER_HPP
#define KAGOME_CORE_LIBP2P_UPGRADER_CONNECTION_UPGRADER_HPP

#include <memory>

#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/transport/muxed_connection.hpp"

namespace libp2p::upgrader {
  /**
   * @brief connection type
   */
  enum class ConnectionType { SERVER_SIDE, CLIENT_SIDE };

  /**
   * @class MuxerOptions muxer options
   */
  struct MuxerOptions {
    ConnectionType connection_type_;
  };

  /**
   * @class ConnectionUpgrader connection upgrader
   */
  class ConnectionUpgrader {
   public:
    virtual ~ConnectionUpgrader() = default;

    using NewStreamHandler = muxer::Yamux::NewStreamHandler;

    /**
     * @brief upgrades connection to muxed
     * @param connection shared ptr to connection instance
     * @param muxer_options Muxer configuration options
     * @param handler generic protocol handler
     * @return muxed connection instance
     */
    virtual std::unique_ptr<transport::MuxedConnection>
    upgradeToMuxed(std::shared_ptr<transport::Connection> connection,
                   MuxerOptions muxer_options,
                   NewStreamHandler handler) const = 0;

    // TODO(yuraz): PRE-149 implement upgradeToSecure()
  };
}  // namespace libp2p::upgrader

#endif  // KAGOME_CORE_LIBP2P_UPGRADER_CONNECTION_UPGRADER_HPP
