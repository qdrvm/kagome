/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/trie_batches.hpp"

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>

#include "log/logger.hpp"
#include "storage/trie/impl/direct_storage.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {
  class Codec;
  class PolkadotTrie;

  class TrieBatchBase : public TrieBatch {
   public:
    TrieBatchBase(std::shared_ptr<Codec> codec,
                  std::shared_ptr<TrieSerializer> serializer,
                  std::shared_ptr<PolkadotTrie> trie);

    TrieBatchBase(std::shared_ptr<Codec> codec,
                  std::shared_ptr<TrieSerializer> serializer,
                  std::shared_ptr<PolkadotTrie> trie,
                  std::shared_ptr<DirectStorage> direct_kv_storage,
                  std::shared_ptr<DirectStorageView> direct_storage_view);

    TrieBatchBase(const TrieBatchBase &) = delete;
    TrieBatchBase(TrieBatchBase &&) noexcept = default;

    ~TrieBatchBase() override = default;

    TrieBatchBase &operator=(const TrieBatchBase &) = delete;
    TrieBatchBase &operator=(TrieBatchBase &&) noexcept = default;

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;
    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    outcome::result<bool> contains(const BufferView &key) const override;

    outcome::result<std::optional<std::shared_ptr<TrieBatch>>> createChildBatch(
        common::BufferView path) override;

   protected:
    virtual outcome::result<std::unique_ptr<TrieBatchBase>> createFromTrieHash(
        const RootHash &trie_hash) = 0;

    outcome::result<void> commitChildren(StateVersion version);

    log::Logger logger_ = log::createLogger("TrieBatch", "storage");

    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    std::shared_ptr<PolkadotTrie> trie_;

    struct DirectStorageStuff {
      // direct storage manages overall logic
      std::shared_ptr<DirectStorage> storage;
      // view belongs to a particular state
      std::shared_ptr<DirectStorageView> view;
      // diff accumulates current changes
      StateDiff diff;
    };
    std::optional<DirectStorageStuff> direct_;

   private:
    std::unordered_map<common::Buffer, std::shared_ptr<TrieBatchBase>>
        child_batches_;
  };

}  // namespace kagome::storage::trie
