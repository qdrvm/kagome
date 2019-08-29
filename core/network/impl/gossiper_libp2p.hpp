/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_LIBP2P_HPP
#define KAGOME_GOSSIPER_LIBP2P_HPP

#include <memory>

#include "common/logger.hpp"
#include "crypto/random_generator.hpp"
#include "network/gossiper.hpp"
#include "network/gossiper_config.hpp"
#include "network/network_state.hpp"

namespace kagome::network {
  struct GossiperClient;

  class GossiperLibp2p : public Gossiper,
                         public std::enable_shared_from_this<GossiperLibp2p> {
   public:
    GossiperLibp2p(std::shared_ptr<NetworkState> network_state,
                   GossiperConfig config,
                   std::shared_ptr<crypto::RandomGenerator> random_generator,
                   common::Logger log = common::createLogger("GossiperLibp2p"));

    ~GossiperLibp2p() override = default;

    void blockAnnounce(
        BlockAnnounce block_announce,
        std::function<void(const outcome::result<void> &)> cb) const override;

    enum class Error { SUCCESS = 0, INVALID_CONFIG = 1 };

   private:
    /**
     * Get a set of peers, chosen by RandomN strategy
     * @return set of peers
     */
    std::vector<std::shared_ptr<GossiperClient>> strategyRandomN() const;

    std::shared_ptr<NetworkState> network_state_;
    GossiperConfig config_;
    std::shared_ptr<crypto::RandomGenerator> random_generator_;
    common::Logger log_;
  };
}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, GossiperLibp2p::Error)

#endif  // KAGOME_GOSSIPER_LIBP2P_HPP
