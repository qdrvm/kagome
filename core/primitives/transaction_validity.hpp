/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP_
#define KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP_

#include <outcome/outcome-register.hpp>
#include <vector>

namespace kagome::primitives {

  /// Priority for a transaction. Additive. Higher is better.
  using TransactionPriority = uint64_t;

  /// Minimum number of blocks a transaction will remain valid for.
  /// `TransactionLongevity::max_value()` means "forever".
  using TransactionLongevity = uint64_t;

  /// Tag for a transaction. No two transactions with the same tag should be
  /// placed on-chain.
  using TransactionTag = std::vector<uint8_t>;

  struct Valid {
    /// Priority of the transaction.
    ///
    /// Priority determines the ordering of two transactions that have all
    /// their dependencies (required tags) satisfied.
    TransactionPriority priority_;

    /// Transaction dependencies
    ///
    /// A non-empty list signifies that some other transactions which provide
    /// given tags are required to be included before that one.
    std::vector<TransactionTag> requires_;

    /// Provided tags
    ///
    /// A list of tags this transaction provides. Successfuly importing the
    /// transaction will enable other transactions that depend on (require)
    /// those tags to be included as well. Provided and requried tags allow
    /// Substrate to build a dependency graph of transactions and import them in
    /// the right (linear) order.
    std::vector<TransactionTag> provides_;

    /// Transaction longevity
    ///
    /// Longevity describes minimum number of blocks the validity is correct.
    /// After this period transaction should be removed from the pool or
    /// revalidated.
    TransactionLongevity longevity_;
  };

  enum class TransactionValidityErrc { INVALID, UNKNOWN };

}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, TransactionValidityErrc)

#endif  // KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP_
