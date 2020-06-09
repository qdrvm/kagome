/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH
#define KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH

#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::storage::trie {

  class EphemeralTrieBatchImpl : public EphemeralTrieBatch {
   public:
    EphemeralTrieBatchImpl(std::shared_ptr<Codec> codec,
                           std::unique_ptr<PolkadotTrie> trie);
    ~EphemeralTrieBatchImpl() override = default;

    outcome::result<Buffer> get(const Buffer &key) const override;
    bool contains(const Buffer &key) const override;
    bool empty() const override;
    outcome::result<Buffer> calculateRoot() const override;
    outcome::result<void> clearPrefix(const Buffer &prefix) override;
    outcome::result<void> put(const Buffer &key, const Buffer &value) override;
    outcome::result<void> put(const Buffer &key, Buffer &&value) override;
    outcome::result<void> remove(const Buffer &key) override;

   private:
    std::shared_ptr<Codec> codec_;
    std::unique_ptr<PolkadotTrie> trie_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_EPHEMERAL_TRIE_BATCH
