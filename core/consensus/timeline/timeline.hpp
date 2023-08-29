/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/timeline/sync_state.hpp"

namespace kagome::consensus {

  class Timeline {
   public:
    Timeline() = default;
    virtual ~Timeline() = default;

    Timeline(Timeline &&) noexcept = delete;
    Timeline(const Timeline &) = delete;
    Timeline &operator=(Timeline &&) noexcept = delete;
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
  };

}  // namespace kagome::consensus
