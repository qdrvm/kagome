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
    using TxHash = primitives::Transaction::Hash;
    using ExtrinsicId = common::Blob<sizeof(primitives::BlockNumber)
                                     + sizeof(primitives::ExtrinsicIndex)>;

    ExtrinsicKey subscribeTransaction(const TxHash &tx_hash) noexcept {
      tx_to_key_[tx_hash] = ++last_key_;
      return last_key_;
    }

    ExtrinsicKey upgradeTransaction(
        const TxHash &tx_hash,
        const primitives::BlockNumber &block_number,
        const primitives::ExtrinsicIndex &ext_idx) noexcept {
      auto it = tx_to_key_.find(tx_hash);
      auto ext_key = it->second;
      id_to_key_[combineId(block_number, ext_idx)] = ext_key;
      tx_to_key_.erase(it);
      return ext_key;
    }

    bool dropTransaction(const TxHash &tx_hash) noexcept {
      return tx_to_key_.erase(tx_hash) > 0;
    }

    boost::optional<ExtrinsicKey> getEventKey(
        const primitives::BlockNumber &block_number,
        const primitives::ExtrinsicIndex &ext_idx) const noexcept {
      auto it = id_to_key_.find(combineId(block_number, ext_idx));
      if (it != id_to_key_.end()) {
        return it->second;
      }
      return boost::none;
    }

    boost::optional<ExtrinsicKey> getEventKey(
        const TxHash &tx_hash) const noexcept {
      auto it = tx_to_key_.find(tx_hash);
      if (it != tx_to_key_.end()) {
        return it->second;
      }
      return boost::none;
    }

   private:
    ExtrinsicId combineId(
        const primitives::BlockNumber &block_number,
        const primitives::ExtrinsicIndex &ext_idx) const noexcept {
      ExtrinsicId id;
      std::memcpy(id.data(), &block_number, sizeof(block_number));
      std::memcpy(id.data() + sizeof(block_number), &ext_idx, sizeof(ext_idx));
      return id;
    }

    std::atomic<ExtrinsicKey> last_key_;
    std::unordered_map<TxHash, ExtrinsicKey> tx_to_key_;
    std::unordered_map<ExtrinsicId, ExtrinsicKey> id_to_key_;
  };

}  // namespace kagome::subscription

#endif  // KAGOME_EXTRINSIC_EVENT_KEY_REPOSITORY_HPP
