/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACTION_HPP
#define KAGOME_TRANSACTION_HPP

#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::primitives {

  struct Transaction {
    /// Hash of tx
    using Hash = common::Hash256;

    /// Priority for a transaction. Additive. Higher is better.
    using Priority = uint64_t;

    /**
     * Minimum number of blocks a transaction will remain valid for.
     * `TransactionLongevity::max_value()` means "forever".
     */
    using Longevity = uint64_t;

    /**
     * Tag for a transaction. No two transactions with the same tag should
     * be placed on-chain.
     */
    using Tag = std::vector<uint8_t>;

    /// Raw extrinsic representing that transaction.
    Extrinsic ext{};

    /// Number of bytes encoding of the transaction requires.
    size_t bytes{};

    /// Extrinsic hash (non-unique)
    Hash hash{};

    /// Transaction priority (higher = better)
    Priority priority{};

    /// At which block the transaction becomes invalid?
    Longevity valid_till{};

    // clang-format treats `requires` as c++20 keyword.
    // clang-format off
    /// Tags required by the transaction.
    std::vector<Tag> requires{};
    // clang-format on

    /// Tags that this transaction provides.
    std::vector<Tag> provides{};

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
