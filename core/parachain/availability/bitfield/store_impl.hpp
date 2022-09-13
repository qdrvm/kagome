/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP

#include "parachain/availability/bitfield/store.hpp"

#include <unordered_map>

namespace kagome::parachain {
  class BitfieldStoreImpl : public BitfieldStore {
   public:
    ~BitfieldStoreImpl() override = default;

    void putBitfield(const BlockHash &relay_parent,
                     const SignedBitfield &bitfield) override;
    std::vector<SignedBitfield> getBitfields(
        const BlockHash &relay_parent) const override;

   private:
    std::unordered_map<BlockHash, std::vector<SignedBitfield>> bitfields_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP
