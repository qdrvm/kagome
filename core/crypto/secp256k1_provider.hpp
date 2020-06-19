/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP
#define KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP

#include "crypto/secp256k1_types.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {

  enum class Secp256k1ProviderError {
    INVALID_ARGUMENT = 1,
    RECOVERY_FAILED,
  };

  /**
   * @class Secp256k1Provider provides public key recovery functionality
   */
  class Secp256k1Provider {
   public:
    virtual ~Secp256k1Provider() = default;

    /**
     * @brief recover public key in uncompressed form
     * @param signature signature
     * @param message_hash blake2s message hash
     * @return uncompressed public key or error
     */
    virtual outcome::result<Secp256k1UncompressedPublicKey>
    recoverPublickeyUncompressed(
        const Secp256k1Signature &signature,
        const Secp256k1Message &message_hash) const = 0;

    /**
     * @brief recover public key in compressed form
     * @param signature signature
     * @param message_hash blake2s message hash
     * @return compressed public key or error
     */
    virtual outcome::result<Secp256k1CompressedPublicKey>
    recoverPublickeyCompressed(const Secp256k1Signature &signature,
                               const Secp256k1Message &message_hash) const = 0;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Secp256k1ProviderError);

#endif  // KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP
