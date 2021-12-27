/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP

#include <gsl/span>
#include <outcome/outcome.hpp>

#include "crypto/ecdsa_types.hpp"

namespace kagome::crypto {

  /**
   * ecdsa provider error codes
   */
  enum class EcdsaProviderError {
    SIGN_UNKNOWN_ERROR = 1,  // unknown error occured during call to `sign`
                             // method of bound function
    VERIFY_UNKNOWN_ERROR,    // unknown error occured during call to `verify`
                             // method of bound function
    DERIVE_UNKNOWN_ERROR     // unknown error occured during call to `derive`
                             // method of bound function
  };

  class EcdsaProvider {
   public:
    virtual ~EcdsaProvider() = default;

    virtual EcdsaKeypair generate() const = 0;

    virtual outcome::result<EcdsaPublicKey> derive(
        const EcdsaSeed &seed) const = 0;

    virtual outcome::result<EcdsaSignature> sign(
        gsl::span<const uint8_t> message, const EcdsaPrivateKey &key) const = 0;

    virtual outcome::result<bool> verify(
        gsl::span<const uint8_t> message,
        const EcdsaSignature &signature,
        const EcdsaPublicKey &publicKey) const = 0;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, EcdsaProviderError)

#endif  // KAGOME_CORE_CRYPTO_ECDSA_PROVIDER_HPP
