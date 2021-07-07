/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
#define KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP

#include "outcome/outcome.hpp"
#include "storage/trie/types.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {

  class RuntimeUpgradeTracker {
   public:
    virtual ~RuntimeUpgradeTracker() = default;

    virtual outcome::result<storage::trie::RootHash> getLastCodeUpdateState(
        const primitives::BlockInfo &block) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_HPP
