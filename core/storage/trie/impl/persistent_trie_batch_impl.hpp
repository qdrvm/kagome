/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include "storage/trie/impl/trie_batch_base.hpp"

#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/serialization/codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie_pruner {
  class TriePruner;
}

namespace kagome::storage::trie {

  class PersistentTrieBatchImpl final : public TrieBatchBase {
   public:
    enum class Error : uint8_t {
      NO_TRIE = 1,
    };

    PersistentTrieBatchImpl(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        TrieChangesTrackerOpt changes,
        std::shared_ptr<PolkadotTrie> trie,
        std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner);
    ~PersistentTrieBatchImpl() override = default;

    outcome::result<RootHash> commit(StateVersion version) override;

    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix,
        std::optional<uint64_t> limit = std::nullopt) override;
    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const BufferView &key) override;

   protected:
    outcome::result<std::unique_ptr<TrieBatchBase>> createFromTrieHash(
        const RootHash &trie_hash) override;

   private:
    TrieChangesTrackerOpt changes_;
    std::shared_ptr<storage::trie_pruner::TriePruner> state_pruner_;
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie,
                          PersistentTrieBatchImpl::Error);
