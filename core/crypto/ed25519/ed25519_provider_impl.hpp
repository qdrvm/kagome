/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_ED25519_PROVIDER_IMPL_H
#define KAGOME_CRYPTO_ED25519_PROVIDER_IMPL_H

#include "crypto/ed25519_provider.hpp"

#include "crypto/random_generator.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

  class Ed25519ProviderImpl : public Ed25519Provider {
   public:
    enum class Error { VERIFICATION_FAILED = 1, SIGN_FAILED };

    explicit Ed25519ProviderImpl(std::shared_ptr<CSPRNG> generator);

    Ed25519KeypairAndSeed generateKeypair() const override;

    Ed25519Keypair generateKeypair(const Ed25519Seed &seed) const override;

    outcome::result<Ed25519Signature> sign(
        const Ed25519Keypair &keypair,
        gsl::span<const uint8_t> message) const override;

    outcome::result<bool> verify(
        const Ed25519Signature &signature,
        gsl::span<const uint8_t> message,
        const Ed25519PublicKey &public_key) const override;

   private:
    std::shared_ptr<CSPRNG> generator_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Ed25519ProviderImpl::Error);

#endif  // KAGOME_CRYPTO_ED25519_PROVIDER_IMPL_H
