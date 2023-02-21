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
      TrieSerializer::OnNodeLoaded const &on_child_node_loaded)
      : codec_{std::move(codec)},
        trie_{std::move(trie)},
        serializer_{std::move(serializer)},
        on_child_node_loaded_{on_child_node_loaded} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(trie_ != nullptr);
    BOOST_ASSERT(serializer_ != nullptr);
    // on_child_node_loaded_ can be zero
  }

  outcome::result<BufferOrView> EphemeralTrieBatchImpl::get(
      const BufferView &key) const {
    return trie_->get(key);
  }

  outcome::result<std::optional<BufferOrView>> EphemeralTrieBatchImpl::tryGet(
      const BufferView &key) const {
    return trie_->tryGet(key);
  }

  std::unique_ptr<PolkadotTrieCursor> EphemeralTrieBatchImpl::trieCursor() {
    return std::make_unique<PolkadotTrieCursorImpl>(trie_);
  }

  outcome::result<bool> EphemeralTrieBatchImpl::contains(
      const BufferView &key) const {
    return trie_->contains(key);
  }

  bool EphemeralTrieBatchImpl::empty() const {
    return trie_->empty();
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

  outcome::result<RootHash> EphemeralTrieBatchImpl::hash(StateVersion version) {
    if (auto root = trie_->getRoot()) {
      OUTCOME_TRY(encoded, codec_->encodeNode(*root, version, {}));
      auto hash = codec_->hash256(encoded);
      return hash;
    }
    return kEmptyRootHash;
  }

}  // namespace kagome::storage::trie
