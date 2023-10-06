/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/types/collator_messages.hpp"

namespace kagome::parachain {
  /// Stores bitfields signed by validators.
  class BitfieldStore {
   public:
    using BlockHash = primitives::BlockHash;
    using SignedBitfield = network::SignedBitfield;

    virtual ~BitfieldStore() = default;

    /// Store bitfield at given block.
    virtual void putBitfield(const BlockHash &relay_parent,
                             const SignedBitfield &bitfield) = 0;

    /// Get bitfields for given block.
    virtual std::vector<SignedBitfield> getBitfields(
        const BlockHash &relay_parent) const = 0;

    /// Clears all data relative relay_parent
    virtual void remove(const BlockHash &relay_parent) = 0;
  };
}  // namespace kagome::parachain
