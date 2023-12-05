/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/beefy/types.hpp"
#include "primitives/justification.hpp"

namespace kagome::network {
  class IBeefy {
   public:
    virtual ~IBeefy() = default;

    virtual outcome::result<std::optional<consensus::beefy::BeefyJustification>>
    getJustification(primitives::BlockNumber block) const = 0;

    virtual void onJustification(const primitives::BlockHash &block_hash,
                                 primitives::Justification raw) = 0;
  };
}  // namespace kagome::network
