/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  class Sr25519ProviderImpl : public Sr25519Provider {
   public:
    Sr25519Keypair generateKeypair(const Sr25519Seed &seed,
                                   Junctions junctions) const override;

    outcome::result<Sr25519Signature> sign(
        const Sr25519Keypair &keypair,
        gsl::span<const uint8_t> message) const override;

    outcome::result<bool> verify_deprecated(
        const Sr25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Sr25519PublicKey &public_key) const override;

    outcome::result<bool> verify(
        const Sr25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Sr25519PublicKey &public_key) const override;
  };

}  // namespace kagome::crypto
