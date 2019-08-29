/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include <memory>

#include "common/logger.hpp"
#include "network/gossiper.hpp"
#include "network/network_state.hpp"

namespace kagome::network {
  class GossiperBroadcast
      : public Gossiper,
        public std::enable_shared_from_this<GossiperBroadcast> {
   public:
    explicit GossiperBroadcast(
        std::shared_ptr<NetworkState> network_state,
        common::Logger log = common::createLogger("GossiperLibp2p"));

    ~GossiperBroadcast() override = default;

    void blockAnnounce(BlockAnnounce block_announce) const override;

   private:
    std::shared_ptr<NetworkState> network_state_;
    common::Logger log_;
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
