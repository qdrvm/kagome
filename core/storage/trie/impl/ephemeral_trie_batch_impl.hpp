/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH

#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class EphemeralTrieBatchImpl final : public EphemeralTrieBatch {
   public:
    EphemeralTrieBatchImpl(
        std::shared_ptr<Codec> codec,
        std::shared_ptr<PolkadotTrie> trie,
        std::shared_ptr<TrieSerializer> serializer,
        TrieSerializer::OnNodeLoaded const &on_child_node_loaded);
    ~EphemeralTrieBatchImpl() override = default;

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
    outcome::result<RootHash> hash(StateVersion version) override;

    virtual outcome::result<std::shared_ptr<TrieBatch>> createChildBatch(
        common::Buffer path) override {
      // TODO(Harrm) code duplication here and in persistent batch
      OUTCOME_TRY(child_root_value, tryGet(path));
      auto child_root_hash =
          child_root_value
              ? common::Hash256::fromSpan(*child_root_value).value()
              : serializer_->getEmptyRootHash();
      OUTCOME_TRY(
          child_trie,
          serializer_->retrieveTrie(child_root_hash, on_child_node_loaded_));

      return std::shared_ptr<TrieBatch>(new EphemeralTrieBatchImpl{
          codec_, child_trie, serializer_, on_child_node_loaded_});
    }

   private:
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<PolkadotTrie> trie_;
    std::shared_ptr<TrieSerializer> serializer_;
    TrieSerializer::OnNodeLoaded const &on_child_node_loaded_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH
