/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_BATCH_BASE_HPP
#define KAGOME_TRIE_BATCH_BASE_HPP

#include "storage/trie/trie_batches.hpp"

#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>

#include "log/logger.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {
  class Codec;
  class PolkadotTrie;

  class TrieBatchBase : public TrieBatch {
   public:
    TrieBatchBase(std::shared_ptr<Codec> codec,
                  std::shared_ptr<TrieSerializer> serializer,
                  std::shared_ptr<PolkadotTrie> trie);

    TrieBatchBase(const TrieBatchBase &) = delete;
    TrieBatchBase(TrieBatchBase &&) = default;

    TrieBatchBase &operator=(const TrieBatchBase &) = delete;
    TrieBatchBase &operator=(TrieBatchBase &&) = default;

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;
    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    outcome::result<bool> contains(const BufferView &key) const override;
    bool empty() const override;

    virtual outcome::result<std::optional<std::shared_ptr<TrieBatch>>>
    createChildBatch(common::BufferView path) override;

   protected:
    virtual outcome::result<std::unique_ptr<TrieBatchBase>> createFromTrieHash(
        const RootHash &trie_hash) = 0;

    outcome::result<void> commitChildren(StateVersion version);

    log::Logger logger_ = log::createLogger("TrieBatch", "storage");

    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    std::shared_ptr<PolkadotTrie> trie_;

   private:
    std::unordered_map<common::Buffer, std::shared_ptr<TrieBatchBase>>
        child_batches_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_BATCH_BASE_HPP
