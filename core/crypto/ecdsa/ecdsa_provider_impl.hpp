/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_ECDSA_PROVIDER_IMPL_H
#define KAGOME_CRYPTO_ECDSA_PROVIDER_IMPL_H

#include "crypto/ecdsa_provider.hpp"

#include "libp2p/crypto/ecdsa_provider/ecdsa_provider_impl.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

  class EcdsaProviderImpl : public EcdsaProvider {
   public:
    enum class Error { VERIFICATION_FAILED = 1, SIGN_FAILED };
    using Libp2pEcdsaProvider = libp2p::crypto::ecdsa::EcdsaProvider;
    using Libp2pEcdsaProviderImpl = libp2p::crypto::ecdsa::EcdsaProviderImpl;

    EcdsaProviderImpl();

    explicit EcdsaProviderImpl(std::shared_ptr<Libp2pEcdsaProvider> provider);

    outcome::result<EcdsaKeypair> generate() const override;

    outcome::result<EcdsaPublicKey> derive(
        const EcdsaSeed &seed) const override;

    outcome::result<EcdsaSignature> sign(
        gsl::span<const uint8_t> message,
        const EcdsaPrivateKey &key) const override;

    outcome::result<EcdsaSignature> signPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaPrivateKey &key) const override;

    outcome::result<bool> verify(
        gsl::span<const uint8_t> message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const override;

    outcome::result<bool> verifyPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const override;

   private:
    std::shared_ptr<Libp2pEcdsaProvider> provider_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, EcdsaProviderImpl::Error);

#endif  // KAGOME_CRYPTO_ECDSA_PROVIDER_IMPL_H
