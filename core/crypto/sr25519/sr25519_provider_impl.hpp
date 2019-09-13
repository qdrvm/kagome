/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP

#include "crypto/random_generator.hpp"
#include "crypto/sr25519_provider.hpp"

namespace libp2p::crypto::random {
  class CSPRNG;
}

namespace kagome::crypto {

  enum class SR25519ProviderError {
    SIGN_UNKNOWN_ERROR = 1,
    VERIFY_UNKNOWN_ERROR
  };

  class SR25519ProviderImpl : public SR25519Provider {
    using CSPRNG = libp2p::crypto::random::CSPRNG;

   public:
    explicit SR25519ProviderImpl(std::shared_ptr<CSPRNG> generator);

    ~SR25519ProviderImpl() override = default;

    SR25519Keypair generateKeypair() const override;

    outcome::result<SR25519Signature> sign(
        const SR25519Keypair &keypair,
        gsl::span<uint8_t> message) const override;

    outcome::result<bool> verify(
        const SR25519Signature &signature,
        gsl::span<uint8_t> message,
        const SR25519PublicKey &public_key) const override;

   private:
    std::shared_ptr<CSPRNG> generator_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, SR25519ProviderError)

#endif  // KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
