/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH

#include <memory>

#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class PersistentTrieBatchImpl final : public PersistentTrieBatch {
   public:
    enum class Error {
      NO_TRIE = 1,
    };

    static std::unique_ptr<PersistentTrieBatchImpl> create(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
        std::shared_ptr<PolkadotTrie> trie);
    ~PersistentTrieBatchImpl() override = default;

    outcome::result<RootHash> commit(StateVersion version) override;
    std::unique_ptr<TopperTrieBatch> batchOnTop() override;

    outcome::result<BufferOrView> get(const BufferView &key) const override;
    outcome::result<std::optional<BufferOrView>> tryGet(
        const BufferView &key) const override;
    std::unique_ptr<PolkadotTrieCursor> trieCursor() override;
    outcome::result<bool> contains(const BufferView &key) const override;
    bool empty() const override;
    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix,
        std::optional<uint64_t> limit = std::nullopt) override;
    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const BufferView &key) override;

    virtual outcome::result<std::shared_ptr<TrieBatch>> createChildBatch(
        common::Buffer path) override {
      // TODO(Harrm) code duplication here and in ephemeral batch
      OUTCOME_TRY(child_root_value, tryGet(path));
      auto child_root_hash =
          child_root_value
              ? common::Hash256::fromSpan(*child_root_value).value()
              : serializer_->getEmptyRootHash();
      // can't read trie proofs from persistent batches, because don't need to
      OUTCOME_TRY(child_trie,
                  serializer_->retrieveTrie(child_root_hash, nullptr));

      auto [it, success] = child_batches_.insert(
          {path,
           std::shared_ptr<PersistentTrieBatchImpl>(new PersistentTrieBatchImpl{
               codec_, serializer_, changes_, child_trie})});
      BOOST_ASSERT(success);
      return it->second;
    }

   private:
    PersistentTrieBatchImpl(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes,
        std::shared_ptr<PolkadotTrie> trie);

    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    std::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes_;
    std::shared_ptr<PolkadotTrie> trie_;

    std::unordered_map<common::Buffer, std::shared_ptr<PersistentTrieBatch>>
        child_batches_;

    log::Logger logger_ = log::createLogger("PersistentTrieBatch", "storage");
  };

}  // namespace kagome::storage::trie

OUTCOME_HPP_DECLARE_ERROR(kagome::storage::trie,
                          PersistentTrieBatchImpl::Error);

#endif  // KAGOME_STORAGE_TRIE_IMPL_PERSISTENT_TRIE_BATCH
