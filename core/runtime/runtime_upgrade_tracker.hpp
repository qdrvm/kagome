/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP

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
     * @return the latest state earlier than the state of \param block, where
     * runtime upgrade happened
     */
    virtual outcome::result<storage::trie::RootHash> getLastCodeUpdateState(
        const primitives::BlockInfo &block) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
