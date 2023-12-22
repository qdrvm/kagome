/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/secp256k1_types.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {

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
    virtual outcome::result<secp256k1::UncompressedPublicKey>
    recoverPublickeyUncompressed(const secp256k1::RSVSignature &signature,
                                 const secp256k1::MessageHash &message_hash,
                                 bool allow_overflow) const = 0;

    /**
     * @brief recover public key in compressed form
     * @param signature signature
     * @param message_hash blake2s message hash
     * @return compressed public key or error
     */
    virtual outcome::result<secp256k1::CompressedPublicKey>
    recoverPublickeyCompressed(const secp256k1::RSVSignature &signature,
                               const secp256k1::MessageHash &message_hash,
                               bool allow_overflow) const = 0;
  };

}  // namespace kagome::crypto
