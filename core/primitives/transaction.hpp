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
    size_t bytes{};

    /// Transaction hash (unique)
    common::Hash256 hash;

    /// Transaction priority (higher = better)
    TransactionPriority priority{};

    /// At which block the transaction becomes invalid?
    TransactionLongevity valid_till{};

    /// Tags required by the transaction.
    std::vector<TransactionTag> requires;

    /// Tags that this transaction provides.
    std::vector<TransactionTag> provides;

    /// Should that transaction be propagated.
    bool should_propagate{false};
  };

  inline bool operator==(const Transaction &v1, const Transaction &v2) {
    return v1.ext == v2.ext && v1.bytes == v2.bytes && v1.hash == v2.hash
        && v1.priority == v2.priority && v1.valid_till == v2.valid_till
        && v1.requires == v2.requires && v1.provides == v2.provides
        && v1.should_propagate == v2.should_propagate;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_TRANSACTION_HPP
