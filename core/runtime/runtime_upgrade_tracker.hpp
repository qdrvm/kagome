/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP

#include "outcome/outcome.hpp"
#include "primitives/block_id.hpp"

namespace kagome::runtime {

  /**
   * Tracks the upgrades of the runtime and supplies the states where it
   * happened
   */
  class RuntimeUpgradeTracker {
   public:
    virtual ~RuntimeUpgradeTracker() = default;

    /**
     * @return the latest block earlier than \param block, where
     * runtime upgrade happened
     */
    virtual outcome::result<primitives::BlockId>
    getRuntimeChangeBlock(const primitives::BlockInfo &block) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
