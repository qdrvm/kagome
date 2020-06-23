/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/ephemeral_trie_batch_impl.hpp"

namespace kagome::storage::trie {

  EphemeralTrieBatchImpl::EphemeralTrieBatchImpl(
      std::shared_ptr<Codec> codec, std::unique_ptr<PolkadotTrie> trie)
      : codec_{std::move(codec)}, trie_{std::move(trie)} {
    BOOST_ASSERT(codec_ != nullptr);
    BOOST_ASSERT(trie_ != nullptr);
  }

  outcome::result<Buffer> EphemeralTrieBatchImpl::get(const Buffer &key) const {
    return trie_->get(key);
  }

  bool EphemeralTrieBatchImpl::contains(const Buffer &key) const {
    return trie_->contains(key);
  }

  bool EphemeralTrieBatchImpl::empty() const {
    return trie_->empty();
  }

  outcome::result<void> EphemeralTrieBatchImpl::clearPrefix(
      const Buffer &prefix) {
    return trie_->clearPrefix(prefix);
  }

  outcome::result<void> EphemeralTrieBatchImpl::put(const Buffer &key,
                                                    const Buffer &value) {
    return trie_->put(key, value);
  }

  outcome::result<void> EphemeralTrieBatchImpl::put(const Buffer &key,
                                                    Buffer &&value) {
    return trie_->put(key, std::move(value));
  }

  outcome::result<void> EphemeralTrieBatchImpl::remove(const Buffer &key) {
    return trie_->remove(key);
  }
}  // namespace kagome::storage::trie
