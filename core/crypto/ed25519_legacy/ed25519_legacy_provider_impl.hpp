/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED25519_ED25519_LEGACY_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_ED25519_ED25519_LEGACY_PROVIDER_IMPL_HPP

#include "crypto/ed25519_provider.hpp"

namespace kagome::crypto {

  class Ed25519LegacyProviderImpl : public Ed25519Provider {
   public:
    ~Ed25519LegacyProviderImpl() override = default;

    outcome::result<Ed25519Keypair> generateKeypair() const override;

    Ed25519Keypair generateKeypair(const Ed25519Seed &seed) const override;

    outcome::result<kagome::crypto::Ed25519Signature> sign(
        const Ed25519Keypair &keypair,
        gsl::span<uint8_t> message) const override;

    outcome::result<bool> verify(
        const ED25519Signature &signature,
        gsl::span<uint8_t> message,
        const ED25519PublicKey &public_key) const override;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_ED25519_LEGACY_PROVIDER_IMPL_HPP
