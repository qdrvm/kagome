/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CONNETCION_UPGRADER_CONNECTION_UPGRADER_HPP
#define KAGOME_CORE_LIBP2P_CONNETCION_UPGRADER_CONNECTION_UPGRADER_HPP

#include <memory>

#include "libp2p/muxer/yamux.hpp"

namespace libp2p::upgrader {

  class MuxedConnection;

  class ConnectionUpgrader {
   public:
    std::shared_ptr<MuxedConnection> upgradeToMuxed() {

    }
  };
}  // namespace libp2p::connection

#endif  // KAGOME_CORE_LIBP2P_CONNETCION_UPGRADER_CONNECTION_UPGRADER_HPP
