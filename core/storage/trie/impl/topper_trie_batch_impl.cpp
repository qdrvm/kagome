/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/impl/topper_trie_batch_impl.hpp"

#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie,
                            TopperTrieBatchImpl::Error,
                            e) {
  using E = kagome::storage::trie::TopperTrieBatchImpl::Error;
  switch (e) {
    case E::PARENT_EXPIRED:
      return "Pointer to the parent batch expired";
  }
  return "Unknown error";
}

namespace kagome::storage::trie {

  TopperTrieBatchImpl::TopperTrieBatchImpl(std::shared_ptr<TrieBatch> parent)
      : parent_(std::move(parent)) {}

  outcome::result<Buffer> TopperTrieBatchImpl::get(const Buffer &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      if (it->second.has_value()) {
        return it->second.value();
      }
      return TrieError::NO_VALUE;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->get(key);
    }
    return Error::PARENT_EXPIRED;
  }

  bool TopperTrieBatchImpl::contains(const Buffer &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      return it->second.has_value();
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->contains(key);
    }
    return false;
  }

  bool TopperTrieBatchImpl::empty() const {
    if (not cache_.empty()
        and std::any_of(cache_.begin(), cache_.end(), [](auto &p) {
              return p.second.has_value();
            })) {
      return false;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->empty();
    }
    return true;
  }

  outcome::result<void> TopperTrieBatchImpl::put(const Buffer &key,
                                                 const Buffer &value) {
    return put(key, Buffer(value));
  }

  outcome::result<void> TopperTrieBatchImpl::put(const Buffer &key,
                                                 Buffer &&value) {
    cache_[key] = std::move(value);
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::remove(const Buffer &key) {
    cache_[key] = boost::none;
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::clearPrefix(const Buffer &prefix) {
    for (auto& p : cache_) {
      if (p.first.subbuffer(0, prefix.size()) == prefix) {
        cache_[p.first] = boost::none;
      }
    }
    // TODO(Harrm) Finish when cursor is merged
    return outcome::success();
  }

  outcome::result<void> TopperTrieBatchImpl::writeBack() {
    /// TODO(Harrm): For review consideration: should we try to roll back if a
    /// put fails? From once side, we should, because safe atomic changes is
    /// the point of the batch From the other, if a storage operation fails
    /// there's no guarantee that rollback will succeed, so why bother
    if (auto p = parent_.lock(); p != nullptr) {
      auto it = cache_.begin();
      for (; it != cache_.end(); it++) {
        if (it->second.has_value()) {
          OUTCOME_TRY(p->put(it->first, it->second.value()));
        } else {
          OUTCOME_TRY(p->remove(it->first));
        }
      }
      return outcome::success();
    }
    return Error::PARENT_EXPIRED;
  }

}  // namespace kagome::storage::trie
