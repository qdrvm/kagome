/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/sync_state.hpp"

namespace kagome::primitives {
  struct BlockHeader;
}

namespace kagome::consensus {

  class Timeline {
   public:
    Timeline() = default;
    virtual ~Timeline() = default;

    Timeline(Timeline &&) = delete;
    Timeline(const Timeline &) = delete;
    Timeline &operator=(Timeline &&) = delete;
    Timeline &operator=(const Timeline &) = delete;

    /**
     * @returns current state
     */
    virtual SyncState getCurrentState() const = 0;

    /**
     * Checks whether the node was in a synchronized state at least once since
     * startup.
     * @return true when current state was ever set to synchronized during the
     * current run, otherwise - false.
     */
    virtual bool wasSynchronized() const = 0;

    /// Check block for possible equivocation and report if any
    virtual void checkAndReportEquivocation(
        const primitives::BlockHeader &header) = 0;
  };

}  // namespace kagome::consensus
