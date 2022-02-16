/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ecdsa/ecdsa_provider_impl.hpp"

namespace kagome::crypto {

  EcdsaProviderImpl::EcdsaProviderImpl()
      : provider_{std::make_shared<Libp2pEcdsaProviderImpl>()} {}

  EcdsaProviderImpl::EcdsaProviderImpl(
      std::shared_ptr<Libp2pEcdsaProvider> provider)
      : provider_{std::move(provider)},
        logger_{log::createLogger("EcdsaProvider", "ecdsa")} {
    BOOST_ASSERT(provider_ != nullptr);
  }

  outcome::result<EcdsaKeypair> EcdsaProviderImpl::generate() const {
    OUTCOME_TRY(key_pair, provider_->generate());
    EcdsaPrivateKey secret_key;
    std::copy(key_pair.private_key.begin(),
              key_pair.private_key.end(),
              secret_key.begin());
    EcdsaPublicKey public_key;
    std::copy(key_pair.public_key.begin(),
              key_pair.public_key.end(),
              public_key.begin());
    return EcdsaKeypair{.secret_key = secret_key, .public_key = public_key};
  }

  outcome::result<EcdsaPublicKey> EcdsaProviderImpl::derive(
      const EcdsaSeed &seed) const {
    // seed here is private key
    OUTCOME_TRY(public_key, provider_->derive(EcdsaPrivateKey{seed}));
    EcdsaPublicKey res;
    std::copy(public_key.begin(), public_key.end(), res.begin());
    return res;
  }

  outcome::result<EcdsaSignature> EcdsaProviderImpl::sign(
      gsl::span<const uint8_t> message, const EcdsaPrivateKey &key) const {
    return provider_->sign(message, key);
  }

  outcome::result<EcdsaSignature> EcdsaProviderImpl::signPrehashed(
      const EcdsaPrehashedMessage &message, const EcdsaPrivateKey &key) const {
    return provider_->signPrehashed(message, key);
  }

  outcome::result<bool> EcdsaProviderImpl::verify(
      gsl::span<const uint8_t> message,
      const EcdsaSignature &signature,
      const EcdsaPublicKey &publicKey) const {
    return provider_->verify(message, signature, publicKey);
  }

  outcome::result<bool> EcdsaProviderImpl::verifyPrehashed(
      const EcdsaPrehashedMessage &message,
      const EcdsaSignature &signature,
      const EcdsaPublicKey &publicKey) const {
    return provider_->verifyPrehashed(message, signature, publicKey);
  }
}  // namespace kagome::crypto
