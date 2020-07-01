/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP

#include "crypto/ed25519_provider.hpp"

namespace kagome::crypto {

  class ED25519ProviderImpl : public ED25519Provider {
   public:
    ~ED25519ProviderImpl() override = default;

    outcome::result<ED25519Keypair> generateKeypair() const override;

    outcome::result<ED25519Keypair> generateKeypair(
        const ED25519Seed &seed) const override;

    outcome::result<kagome::crypto::ED25519Signature> sign(
        const ED25519Keypair &keypair,
        gsl::span<uint8_t> message) const override;

    outcome::result<bool> verify(
        const ED25519Signature &signature,
        gsl::span<uint8_t> message,
        const ED25519PublicKey &public_key) const override;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED25519_ED25519_PROVIDER_IMPL_HPP
