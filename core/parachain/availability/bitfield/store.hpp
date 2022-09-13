/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_HPP

#include "network/types/collator_messages.hpp"

namespace kagome::parachain {
  class BitfieldStore {
   public:
    using BlockHash = primitives::BlockHash;
    using SignedBitfield = network::SignedBitfield;

    virtual ~BitfieldStore() = 0;

    virtual void putBitfield(const BlockHash &relay_parent,
                             const SignedBitfield &bitfield) = 0;
    virtual std::vector<SignedBitfield> getBitfields(
        const BlockHash &relay_parent) const = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_HPP
