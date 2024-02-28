/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "primitives/justification.hpp"

namespace kagome::network {
  class Beefy {
   public:
    virtual ~Beefy() = default;

    virtual primitives::BlockNumber finalized() const = 0;

    virtual outcome::result<std::optional<consensus::beefy::BeefyJustification>>
    getJustification(primitives::BlockNumber block) const = 0;

    virtual void onJustification(const primitives::BlockHash &block_hash,
                                 primitives::Justification raw) = 0;

    virtual void onMessage(consensus::beefy::BeefyGossipMessage message) = 0;
  };
}  // namespace kagome::network
