/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/trie/impl/trie_batch_base.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/serialization/codec.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class EphemeralTrieBatchImpl final : public TrieBatchBase {
   public:
    EphemeralTrieBatchImpl(std::shared_ptr<Codec> codec,
                           std::shared_ptr<PolkadotTrie> trie,
                           std::shared_ptr<TrieSerializer> serializer,
                           TrieSerializer::OnNodeLoaded on_child_node_loaded);
    ~EphemeralTrieBatchImpl() override = default;

    outcome::result<std::tuple<bool, uint32_t>> clearPrefix(
        const BufferView &prefix,
        std::optional<uint64_t> limit = std::nullopt) override;
    outcome::result<void> put(const BufferView &key,
                              BufferOrView &&value) override;
    outcome::result<void> remove(const BufferView &key) override;
    outcome::result<RootHash> commit(StateVersion version) override;

   protected:
    outcome::result<std::unique_ptr<TrieBatchBase>> createFromTrieHash(
        const RootHash &trie_hash) override;

   private:
    TrieSerializer::OnNodeLoaded on_child_node_loaded_;
  };

}  // namespace kagome::storage::trie
