/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_MOCK_HPP

#include "runtime/runtime_upgrade_tracker.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimeUpgradeTrackerMock : public RuntimeUpgradeTracker {
   public:
    MOCK_METHOD(outcome::result<storage::trie::RootHash>,
                getLastCodeUpdateState,
                (const primitives::BlockInfo &block),
                (override));

    MOCK_METHOD(outcome::result<primitives::BlockInfo>,
                getLastCodeUpdateBlockInfo,
                (const storage::trie::RootHash &state),
                (const, override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_RUNTIME_UPGRADE_TRACKER_MOCK_HPP
