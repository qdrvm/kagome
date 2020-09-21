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

  TopperTrieBatchImpl::TopperTrieBatchImpl(
      const std::shared_ptr<TrieBatch> &parent)
      : parent_(parent) {}

  outcome::result<Buffer> TopperTrieBatchImpl::get(const Buffer &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      if (it->second.has_value()) {
        return it->second.value();
      }
      return TrieError::NO_VALUE;
    }
    if (wasClearedByPrefix(key)) {
      return TrieError::NO_VALUE;
    }
    if (auto p = parent_.lock(); p != nullptr) {
      return p->get(key);
    }
    return Error::PARENT_EXPIRED;
  }

  std::unique_ptr<PolkadotTrieCursor> TopperTrieBatchImpl::trieCursor() {
    if (auto p = parent_.lock(); p != nullptr) {
      return p->trieCursor();
    }
    return nullptr;
  }

  bool TopperTrieBatchImpl::contains(const Buffer &key) const {
    if (auto it = cache_.find(key); it != cache_.end()) {
      return it->second.has_value();
    }
    if (wasClearedByPrefix(key)) {
      return false;
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
    // TODO(Harrm) PRE-462 consider clearPrefix here. Not an easy thing and is
    // barely possible to happen, so leave it for the future
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
    for (auto &p : cache_) {
      if (p.first.subbuffer(0, prefix.size()) == prefix) {
        cache_[p.first] = boost::none;
      }
    }
    cleared_prefixes_.push_back(prefix);
    if (parent_.lock() != nullptr) {
      return outcome::success();
    }
    return Error::PARENT_EXPIRED;
  }

  outcome::result<void> TopperTrieBatchImpl::writeBack() {
    if (auto p = parent_.lock(); p != nullptr) {
      auto it = cache_.begin();
      for (auto &prefix : cleared_prefixes_) {
        OUTCOME_TRY(p->clearPrefix(prefix));
      }
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

  bool TopperTrieBatchImpl::wasClearedByPrefix(const Buffer &key) const {
    for (auto prefix : cleared_prefixes_) {
      auto key_end = key.begin();
      std::advance(key_end, std::min(key.size(), prefix.size()) - 1);
      auto is_cleared = std::equal(key.begin(), key_end, prefix.begin());
      if (is_cleared) return true;
    }
    return false;
  }

}  // namespace kagome::storage::trie
