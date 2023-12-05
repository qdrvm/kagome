/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  /**
   * Tracks the upgrades of the runtime and supplies the states where it
   * happened
   */
  class RuntimeUpgradeTracker {
   public:
    virtual ~RuntimeUpgradeTracker() = default;

    /**
     * @return the latest block earlier or equal to \param block, where
     * runtime upgrade happened
     */
    virtual outcome::result<storage::trie::RootHash> getLastCodeUpdateState(
        const primitives::BlockInfo &block) = 0;

    virtual outcome::result<primitives::BlockInfo> getLastCodeUpdateBlockInfo(
        const storage::trie::RootHash &state) const = 0;
  };

  enum class RuntimeUpgradeTrackerError { NOT_FOUND = 1 };

}  // namespace kagome::runtime

OUTCOME_HPP_DECLARE_ERROR(kagome::runtime, RuntimeUpgradeTrackerError)
