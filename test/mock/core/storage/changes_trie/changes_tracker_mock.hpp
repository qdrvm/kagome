/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK

#include "storage/changes_trie/changes_tracker.hpp"

#include <gmock/gmock.h>

namespace kagome::storage::changes_trie {

  class ChangesTrackerMock : public ChangesTracker {
   public:
    MOCK_METHOD1(setBlockHash, void (const primitives::BlockHash &hash));

    MOCK_METHOD1(setConfig, void (const ChangesTrieConfig &conf));

    MOCK_METHOD1(onBlockChange, outcome::result<void> (
        const primitives::BlockHash &key));

    MOCK_METHOD1(onChange, outcome::result<void> (const common::Buffer &key));

    MOCK_METHOD1(sinkToChangesTrie, outcome::result<void> (
        ChangesTrieBuilder &builder));
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK
