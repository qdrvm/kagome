/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
