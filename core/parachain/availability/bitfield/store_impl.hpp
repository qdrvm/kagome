/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP

#include "log/logger.hpp"
#include "parachain/availability/bitfield/store.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

#include <unordered_map>

namespace kagome::parachain {
  class BitfieldStoreImpl : public BitfieldStore {
   public:
    BitfieldStoreImpl(std::shared_ptr<runtime::ParachainHost> parachain_api);
    ~BitfieldStoreImpl() override = default;

    void putBitfield(const BlockHash &relay_parent,
                     const SignedBitfield &bitfield) override;
    std::vector<SignedBitfield> getBitfields(
        const BlockHash &relay_parent) const override;
    void remove(const BlockHash &relay_parent) override;

   private:
    std::unordered_map<BlockHash, std::vector<SignedBitfield>> bitfields_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    log::Logger logger_ = log::createLogger("BitfieldStore", "parachain");
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_STORE_IMPL_HPP
