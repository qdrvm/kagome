/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bandersnatch_provider.hpp"
#include "crypto/bandersnatch_types.hpp"

namespace kagome::crypto {

  class BandersnatchProviderImpl final : public BandersnatchProvider {
   public:
    BandersnatchKeypair generateKeypair(const BandersnatchSeed &seed,
                                        Junctions junctions) const override;

    outcome::result<BandersnatchSignature> sign(
        const BandersnatchKeypair &keypair,
        common::BufferView message) const override;

    outcome::result<bool> verify(
        const BandersnatchSignature &signature,
        common::BufferView message,
        const BandersnatchPublicKey &public_key) const override;
  };

}  // namespace kagome::crypto
