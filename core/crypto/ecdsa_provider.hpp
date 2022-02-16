/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP

#include <gsl/span>

#include "outcome/outcome.hpp"

#include "crypto/ecdsa_types.hpp"

namespace kagome::crypto {

  class EcdsaProvider {
   public:
    virtual ~EcdsaProvider() = default;

    virtual outcome::result<EcdsaKeypair> generate() const = 0;

    virtual outcome::result<EcdsaPublicKey> derive(
        const EcdsaSeed &seed) const = 0;

    virtual outcome::result<EcdsaSignature> sign(
        gsl::span<const uint8_t> message, const EcdsaPrivateKey &key) const = 0;

    virtual outcome::result<EcdsaSignature> signPrehashed(
        const EcdsaPrehashedMessage &message, const EcdsaPrivateKey &key) const = 0;

    virtual outcome::result<bool> verify(
        gsl::span<const uint8_t> message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const = 0;

    virtual outcome::result<bool> verifyPrehashed(
        const EcdsaPrehashedMessage &message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP
