/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/pool_moderator_impl.hpp"

using kagome::common::Buffer;

namespace kagome::transaction_pool {

  PoolModeratorImpl::PoolModeratorImpl(std::shared_ptr<Clock> clock,
                                       Clock::Duration ban_for,
                                       size_t expected_size)
      : banned_until_{Compare},
        ban_for_{ban_for},
        clock_{std::move(clock)},
        expected_size_{expected_size} {}

  void PoolModeratorImpl::ban(const primitives::Transaction &tx) {
    banned_until_.insert({tx.hash, clock_->now() + ban_for_});
    if (banned_until_.size() > expected_size_ * 2) {
      while (banned_until_.size() > expected_size_) {
        banned_until_.erase(banned_until_.begin());
      }
    }
  }

  bool PoolModeratorImpl::banIfStale(primitives::BlockNumber current_block,
                                     const primitives::Transaction &tx) {
    if (tx.valid_till > current_block) {
      return false;
    }
    ban(tx);
    return true;
  }

  bool PoolModeratorImpl::isBanned(const primitives::Transaction &tx) const {
    auto it = banned_until_.find(tx.hash);
    if (it == banned_until_.end()) {
      return false;
    }
    // if ban time is exceeded, the transaction will be removed from the list
    // on next updateBan()
    return it->second >= clock_->now();
  }

  void PoolModeratorImpl::updateBan() {
    auto now = clock_->now();
    std::list<Map::iterator> removed;
    for (auto it = banned_until_.begin(); it != banned_until_.end(); it++) {
      if (it->second < now) {
        removed.push_back(it);
      }
    }
    for (auto &it : removed) {
      banned_until_.erase(it);
    }
  }

  bool PoolModeratorImpl::Compare(const Buffer &b1, const Buffer &b2) {
    if (b1.size() == b2.size()) {
      for (size_t i = 0; i < b1.size(); i++) {
        if (b1[i] != b2[i]) {
          return b1[i] > b2[i];
        }
      }
    }
    return b1.size() > b2.size();
  }

}  // namespace kagome::transaction_pool
