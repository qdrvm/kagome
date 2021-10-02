/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_RUNTIME_RUNTIME_UPGRADE_TRACKER_MOCK_HPP

#include "runtime/runtime_upgrade_tracker.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class RuntimeUpgradeTrackerMock : public RuntimeUpgradeTracker {
   public:
    MOCK_METHOD1(getLastCodeUpdateState,
                 outcome::result<storage::trie::RootHash>(
                     const primitives::BlockInfo &block));
    MOCK_CONST_METHOD1(getLastCodeUpdateHash,
                       outcome::result<primitives::BlockHash>(
                           const storage::trie::RootHash &state));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_TEST_MOCK_CORE_RUNTIME_WAVM_RUNTIME_UPGRADE_TRACKER_MOCK_HPP
