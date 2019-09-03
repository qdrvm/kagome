/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GOSSIPER_BROADCAST_HPP
#define KAGOME_GOSSIPER_BROADCAST_HPP

#include "network/gossiper.hpp"

namespace kagome::network {
  /**
   * Sends gossip messages using broadcast strategy
   */
  class GossiperBroadcast : public Gossiper {
    using Precommit = consensus::grandpa::Precommit;
    using Prevote = consensus::grandpa::Prevote;
    using PrimaryPropose = consensus::grandpa::PrimaryPropose;

   public:
    ~GossiperBroadcast() override = default;

    void blockAnnounce(const BlockAnnounce &announce) const override {}

    void precommit(Precommit pc) const override {}

    void prevote(Prevote pv) const override {}

    void primaryPropose(PrimaryPropose pv) const override {}
  };
}  // namespace kagome::network

#endif  // KAGOME_GOSSIPER_BROADCAST_HPP
