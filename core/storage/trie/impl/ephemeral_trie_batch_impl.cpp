/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/ephemeral_trie_batch_impl.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

namespace kagome::storage::trie {

  EphemeralTrieBatchImpl::EphemeralTrieBatchImpl(
      std::shared_ptr<Codec> codec,
      std::shared_ptr<PolkadotTrie> trie,
      std::shared_ptr<TrieSerializer> serializer,
      TrieSerializer::OnNodeLoaded on_child_node_loaded)
      : TrieBatchBase{std::move(codec), std::move(serializer), std::move(trie)},
        on_child_node_loaded_{std::move(on_child_node_loaded)} {
    // on_child_node_loaded_ can be zero
  }

  outcome::result<std::tuple<bool, uint32_t>>
  EphemeralTrieBatchImpl::clearPrefix(const BufferView &prefix,
                                      std::optional<uint64_t> limit) {
    return trie_->clearPrefix(prefix, limit, [](const auto &, auto &&) {
      return outcome::success();
    });
  }

  outcome::result<void> EphemeralTrieBatchImpl::put(const BufferView &key,
                                                    BufferOrView &&value) {
    return trie_->put(key, std::move(value));
  }

  outcome::result<void> EphemeralTrieBatchImpl::remove(const BufferView &key) {
    return trie_->remove(key);
  }

  outcome::result<RootHash> EphemeralTrieBatchImpl::commit(
      StateVersion version) {
    OUTCOME_TRY(commitChildren(version));
    if (auto root = trie_->getRoot()) {
      OUTCOME_TRY(encoded, codec_->encodeNode(*root, version, {}));
      auto hash = codec_->hash256(encoded);
      return hash;
    }
    return kEmptyRootHash;
  }

  outcome::result<std::unique_ptr<TrieBatch>>
  EphemeralTrieBatchImpl::createFromTrieHash(const RootHash &trie_hash) {
    OUTCOME_TRY(trie, serializer_->retrieveTrie(trie_hash, nullptr));
    return std::make_unique<EphemeralTrieBatchImpl>(
        codec_, trie, serializer_, on_child_node_loaded_);
  }

}  // namespace kagome::storage::trie
