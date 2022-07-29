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
    MOCK_METHOD(void, setBlockHash, (const primitives::BlockHash &hash), ());

    MOCK_METHOD(outcome::result<void>,
                onBlockExecutionStart,
                (primitives::BlockHash new_parent_hash,
                 primitives::BlockNumber new_parent_number),
                (override));

    MOCK_METHOD(void,
                onBlockAdded,
                (const primitives::BlockHash &block_hash),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onPut,
                (common::BufferView ext_idx,
                 const common::BufferView &key,
                 const common::BufferView &value,
                 bool is_new_entry),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onRemove,
                (common::BufferView ext_idx, const common::BufferView &key),
                (override));
  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_CHANGES_TRIE_CHANGES_TRACKER_MOCK
