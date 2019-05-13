/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/upgrader/impl/connection_upgrader_impl.hpp"

#include "libp2p/muxer/yamux/yamux.hpp"
#include "libp2p/muxer/yamux/yamux_config.hpp"

namespace libp2p::upgrader {

  std::unique_ptr<muxer::StreamMuxer>
  ConnectionUpgraderImpl::upgradeToMuxed(
      std::shared_ptr<transport::Connection> connection,
      MuxerOptions muxer_options,
      ConnectionUpgrader::NewStreamHandler handler) const {
    muxer::YamuxConfig yamux_config = {muxer_options.connection_type_
                                       == ConnectionType::SERVER_SIDE};

    return std::make_unique<muxer::Yamux>(connection, handler,
                                          yamux_config);  // use yamux logger
  }
}  // namespace libp2p::upgrader
