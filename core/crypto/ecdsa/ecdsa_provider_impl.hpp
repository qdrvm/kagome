/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/ecdsa_provider.hpp"

#include <secp256k1.h>

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {
  class Hasher;

  class EcdsaProviderImpl : public EcdsaProvider {
   public:
    enum class Error {
      VERIFICATION_FAILED = 1,
      SIGN_FAILED,
      DERIVE_FAILED,
      SOFT_JUNCTION_NOT_SUPPORTED,
    };

    explicit EcdsaProviderImpl(std::shared_ptr<Hasher> hasher);

    outcome::result<EcdsaKeypair> generateKeypair(
        const EcdsaSeed &seed, Junctions junctions) const override;

    outcome::result<EcdsaSignature> sign(
        common::BufferView message, const EcdsaPrivateKey &key) const override;

    outcome::result<EcdsaSignature> signPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaPrivateKey &key) const override;

    outcome::result<bool> verify(
        common::BufferView message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const override;

    outcome::result<bool> verifyPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const override;

   private:
    std::unique_ptr<secp256k1_context, void (*)(secp256k1_context *)> context_;
    std::shared_ptr<Hasher> hasher_;
    log::Logger logger_;
    Secp256k1ProviderImpl recovery_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, EcdsaProviderImpl::Error);
