/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  /**
   * Fetch chunk for availability bitfield voting.
   */
  class Fetch {
   public:
    virtual ~Fetch() = default;

    virtual void fetch(ValidatorIndex chunk_index,
                       const runtime::OccupiedCore &core,
                       const runtime::SessionInfo &session) = 0;
  };
}  // namespace kagome::parachain
