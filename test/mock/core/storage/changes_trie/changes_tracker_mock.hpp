/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK

#include <gmock/gmock.h>

#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::storage::changes_trie {

  class ChangesTrackerMock : public ChangesTracker {
   public:
    MOCK_METHOD1(setBlockHash, void(const primitives::BlockHash &hash));

    MOCK_METHOD1(setExtrinsicIdxGetter, void(GetExtrinsicIndexDelegate f));

    MOCK_METHOD1(setConfig, void(const ChangesTrieConfig &conf));

    MOCK_METHOD2(
        onBlockChange,
        outcome::result<void>(primitives::BlockHash new_parent_hash,
                              primitives::BlockNumber new_parent_number));

    MOCK_METHOD3(onPut,
                 outcome::result<void>(const common::Buffer &key,
                                       const common::Buffer &value,
                                       bool is_new_entry));
    MOCK_METHOD1(onRemove, outcome::result<void>(const common::Buffer &key));

    MOCK_METHOD2(
        constructChangesTrie,
        outcome::result<common::Hash256>(const primitives::BlockHash &parent,
                                         const ChangesTrieConfig &conf));
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK
