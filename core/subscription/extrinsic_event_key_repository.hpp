/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EXTRINSIC_EVENT_KEY_REPOSITORY_HPP
#define KAGOME_EXTRINSIC_EVENT_KEY_REPOSITORY_HPP

#include <primitives/transaction.hpp>
#include "primitives/event_types.hpp"

namespace kagome::subscription {

  class ExtrinsicEventKeyRepository {
   public:
    using ExtrinsicKey = primitives::events::SubscribedExtrinsicId;

    ExtrinsicEventKeyRepository()
        : logger_{log::createLogger("ExtrinsicEventKeyRepo", "transactions")} {}

    ExtrinsicKey add(const primitives::Transaction::Hash &hash) noexcept {
      if (auto it = keys_.find(hash); it != keys_.end()) {
        return it->second;
      }
      auto key = last_key_++;
      keys_[hash] = key;
      SL_DEBUG(logger_, "Registered tx {}, key is {}", hash, key);
      return key;
    }

    bool remove(const primitives::Transaction::Hash &hash) noexcept {
      return keys_.erase(hash) > 0;
    }

    boost::optional<ExtrinsicKey> get(
        const primitives::Transaction::Hash &hash) const noexcept {
      if (auto it = keys_.find(hash); it != keys_.end()) {
        return it->second;
      }
      return boost::none;
    }

   private:
    std::atomic<ExtrinsicKey> last_key_{};
    std::unordered_map<primitives::Transaction::Hash, ExtrinsicKey> keys_;
    log::Logger logger_;
  };

}  // namespace kagome::subscription

#endif  // KAGOME_EXTRINSIC_EVENT_KEY_REPOSITORY_HPP
