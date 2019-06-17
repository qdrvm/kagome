/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_HPP
#define KAGOME_TRANSACTION_HPP

#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {

  /// Priority for a transaction. Additive. Higher is better.
  using TransactionPriority = uint64_t;

  /**
   * Minimum number of blocks a transaction will remain valid for.
   * `TransactionLongevity::max_value()` means "forever".
   */
  using TransactionLongevity = uint64_t;

  /**
   * Tag for a transaction. No two transactions with the same tag should
   * be placed on-chain.
   */
  using TransactionTag = std::vector<uint8_t>;

  struct Transaction {
    /// Raw extrinsic representing that transaction.
    primitives::Extrinsic ext;

    /// Number of bytes encoding of the transaction requires.
    size_t bytes;

    /// Transaction hash (unique)
    common::Buffer hash;

    /// Transaction priority (higher = better)
    TransactionPriority priority;

    /// At which block the transaction becomes invalid?
    TransactionLongevity valid_till;

    /// Tags required by the transaction.
    std::vector<TransactionTag> requires;

    /// Tags that this transaction provides.
    std::vector<TransactionTag> provides;

    /// Should that transaction be propagated.
    bool should_propogate;
  };

}  // namespace kagome::primitives

#endif  // KAGOME_TRANSACTION_HPP
