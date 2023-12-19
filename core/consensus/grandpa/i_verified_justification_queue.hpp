/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace kagome::consensus::grandpa {
  struct GrandpaJustification;

  using AuthoritySetId = uint64_t;

  /// Finalize scheduled change after finalizing it's parent.
  class IVerifiedJustificationQueue {
   public:
    virtual ~IVerifiedJustificationQueue() = default;

    virtual void addVerified(AuthoritySetId set,
                             GrandpaJustification justification) = 0;

    virtual void warp() = 0;
  };
}  // namespace kagome::consensus::grandpa
