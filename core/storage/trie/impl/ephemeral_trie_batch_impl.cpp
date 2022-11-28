/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/ephemeral_trie_batch_impl.hpp"

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

namespace kagome::storage::trie {

  EphemeralTrieBatchImpl::EphemeralTrieBatchImpl(
      std::shared_ptr<Codec> codec, std::shared_ptr<PolkadotTrie> trie)
      : codec_{std::move(codec)}, trie_{std::move(trie)} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(trie_ != nullptr);
  }

  outcome::result<BufferConstRef> EphemeralTrieBatchImpl::get(
      const BufferView &key) const {
    return trie_->get(key);
  }

  outcome::result<std::optional<BufferConstRef>> EphemeralTrieBatchImpl::tryGet(
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
                                                    const Buffer &value) {
    return trie_->put(key, value);
  }

  outcome::result<void> EphemeralTrieBatchImpl::put(const BufferView &key,
                                                    Buffer &&value) {
    return trie_->put(key, std::move(value));
  }

  outcome::result<void> EphemeralTrieBatchImpl::remove(const BufferView &key) {
    return trie_->remove(key);
  }

  outcome::result<RootHash> EphemeralTrieBatchImpl::hash(StateVersion version) {
    static const auto empty_hash = codec_->hash256(common::Buffer{0});
    if (auto root = trie_->getRoot()) {
      OUTCOME_TRY(encoded, codec_->encodeNode(*root, version, {}));
      auto hash = codec_->hash256(encoded);
      return hash;
    }
    return empty_hash;
  }

}  // namespace kagome::storage::trie
