/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP
#define KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>

namespace kagome::primitives {

  /**
   * This is the same structure as in
   * https://github.com/paritytech/substrate/blob/master/core/sr-primitives/src/transaction_validity.rs
   */

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

  /// Transaction is valid
  struct Valid {
    /**
     * @brief Priority of the transaction.
     * Priority determines the ordering of two transactions that have all
     * their dependencies (required tags) satisfied.
     */
    TransactionPriority priority_;

    /**
     * @brief Transaction dependencies
     * A non-empty list signifies that some other transactions which provide
     * given tags are required to be included before that one.
     */
    std::vector<TransactionTag> requires_;

    /**
     * @brief Provided tags
     * A list of tags this transaction provides. Successfuly importing the
     * transaction will enable other transactions that depend on (require)
     * those tags to be included as well. Provided and requried tags allow
     * Substrate to build a dependency graph of transactions and import them
     * in the right (linear) order.
     */
    std::vector<TransactionTag> provides_;

    /**
     * @brief Transaction longevity
     * Longevity describes minimum number of blocks the validity is correct.
     * After this period transaction should be removed from the pool or
     * revalidated.
     */
    TransactionLongevity longevity_;
  };

  /// Transaction is invalid. Details are described by the error code.
  struct Invalid {
    uint8_t error_;
  };

  /// Transaction validity can't be determined.
  struct Unknown {
    uint8_t error_;
  };

  /**
   * Information on a transaction's validity and, if valid, on how it relate to
   * other transactions.
   */
  using TransactionValidity = boost::variant<Invalid, Valid, Unknown>;

}  // namespace kagome::primitives

namespace kagome::scale {
  class ScaleEncoderStream;

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Invalid &v);

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Valid &v);

  ScaleEncoderStream &operator<<(ScaleEncoderStream &s,
                                 const primitives::Unknown &v);
}  // namespace kagome::scale
#endif  // KAGOME_CORE_PRIMITIVES_TRANSACTION_VALIDITY_HPP
