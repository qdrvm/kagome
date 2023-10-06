/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/ed25519_provider.hpp"

#include "log/logger.hpp"

namespace kagome::crypto {
  class Hasher;

  class Ed25519ProviderImpl : public Ed25519Provider {
   public:
    enum class Error {
      VERIFICATION_FAILED = 1,
      SIGN_FAILED,
      SOFT_JUNCTION_NOT_SUPPORTED,
    };

    Ed25519ProviderImpl(std::shared_ptr<crypto::Hasher> hasher);

    outcome::result<Ed25519Keypair> generateKeypair(
        const Ed25519Seed &seed, Junctions junctions) const override;

    outcome::result<Ed25519Signature> sign(
        const Ed25519Keypair &keypair,
        gsl::span<const uint8_t> message) const override;

    outcome::result<bool> verify(
        const Ed25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Ed25519PublicKey &public_key) const override;

   private:
    std::shared_ptr<crypto::Hasher> hasher_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Ed25519ProviderImpl::Error);
