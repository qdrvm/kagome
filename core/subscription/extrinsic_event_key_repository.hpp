/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <mutex>

#include "primitives/event_types.hpp"
#include "primitives/transaction.hpp"

namespace kagome::subscription {

  class ExtrinsicEventKeyRepository {
   public:
    using ExtrinsicKey = primitives::events::SubscribedExtrinsicId;

    ExtrinsicEventKeyRepository()
        : logger_{log::createLogger("ExtrinsicEventKeyRepo", "transactions")} {}

    ExtrinsicKey add(const primitives::Transaction::Hash &hash) {
      std::unique_lock lock{mutex_};
      if (auto it = keys_.find(hash); it != keys_.end()) {
        return it->second;
      }
      auto key = last_key_++;
      keys_[hash] = key;
      SL_DEBUG(logger_, "Registered tx {}, key is {}", hash, key);
      return key;
    }

    bool remove(const primitives::Transaction::Hash &hash) {
      std::unique_lock lock{mutex_};
      return keys_.erase(hash) > 0;
    }

    std::optional<ExtrinsicKey> get(
        const primitives::Transaction::Hash &hash) const {
      std::unique_lock lock{mutex_};
      if (auto it = keys_.find(hash); it != keys_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

   private:
    mutable std::mutex mutex_;
    std::atomic<ExtrinsicKey> last_key_{};
    std::unordered_map<primitives::Transaction::Hash, ExtrinsicKey> keys_;
    log::Logger logger_;
  };

}  // namespace kagome::subscription
