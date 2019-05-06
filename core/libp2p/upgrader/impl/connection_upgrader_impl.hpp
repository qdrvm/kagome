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
  class ConnectionUpgraderImpl: public ConnectionUpgrader {
   public:
    using NewStreamHandler = muxer::Yamux::NewStreamHandler;

    /**
     * @brief constructor
     * @param muxer_options muxer options
     */
    explicit ConnectionUpgraderImpl(MuxerOptions muxer_options)
        : muxer_options_{muxer_options} {}

    outcome::result<std::unique_ptr<transport::MuxedConnection>> upgradeToMuxed(
        std::shared_ptr<transport::Connection> connection,
        NewStreamHandler handler) const override;

   private:
    MuxerOptions muxer_options_;
  };
}  // namespace libp2p::upgrader

#endif  // KAGOME_CORE_LIBP2P_CONNECTION_UPGRADER_CONNECTION_UPGRADER_IMPL_HPP
