/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "transaction_pool/impl/pool_moderator_impl.hpp"

using kagome::common::Buffer;

namespace kagome::transaction_pool {

  PoolModeratorImpl::PoolModeratorImpl(
      std::shared_ptr<clock::SystemClock> clock, Params parameters)
      : clock_{std::move(clock)}, params_{parameters} {
  }

  void PoolModeratorImpl::ban(const common::Hash256 &tx_hash) {
    banned_until_.insert({tx_hash, clock_->now() + params_.ban_for});
    if (banned_until_.size() > params_.expected_size * 2) {
      while (banned_until_.size() > params_.expected_size) {
        banned_until_.erase(banned_until_.begin());
      }
    }
  }

  bool PoolModeratorImpl::banIfStale(primitives::BlockNumber current_block,
                                     const primitives::Transaction &tx) {
    if (tx.valid_till > current_block) {
      return false;
    }
    ban(tx.hash);
    return true;
  }

  bool PoolModeratorImpl::isBanned(const common::Hash256 &tx_hash) const {
    auto it = banned_until_.find(tx_hash);
    if (it == banned_until_.end()) {
      return false;
    }
    // if ban time is exceeded, the transaction will be removed from the list
    // on next updateBan()
    return it->second >= clock_->now();
  }

  void PoolModeratorImpl::updateBan() {
    auto now = clock_->now();
    for (auto it = banned_until_.begin(); it != banned_until_.end();) {
      if (it->second < now) {
        it = banned_until_.erase(it);
      } else {
        it++;
      }
    }
  }

  size_t PoolModeratorImpl::bannedNum() const {
    return banned_until_.size();
  }

}  // namespace kagome::transaction_pool
