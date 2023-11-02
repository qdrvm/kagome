/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bandersnatch_types.hpp"
#include "crypto/bip39/bip39_types.hpp"

namespace kagome::crypto {

  /**
   * sr25519 provider error codes
   */
  enum class BandersnatchProviderError {
    SIGN_UNKNOWN_ERROR = 1,  // unknown error occured during call to `sign`
                             // method of bound function
    VERIFY_UNKNOWN_ERROR     // unknown error occured during call to `verify`
                             // method of bound function
  };

  class BandersnatchProvider {
   public:
    using Junctions = std::span<const bip39::RawJunction>;

    virtual ~BandersnatchProvider() = default;

    /**
     * Generate random keypair from seed
     */
    virtual BandersnatchKeypair generateKeypair(const BandersnatchSeed &seed,
                                                Junctions junctions) const = 0;

    /**
     * Sign message \param msg using \param keypair. If computed value is less
     * than \param threshold then return optional containing this value and
     * proof. Otherwise none returned
     * @param keypair pair of public and secret sr25519 keys
     * @param message bytes to be signed
     * @return signed message
     */
    virtual outcome::result<BandersnatchSignature> sign(
        const BandersnatchKeypair &keypair,
        common::BufferView message) const = 0;

    /**
     * Verifies that \param message was derived using \param public_key on
     * \param signature
     */
    virtual outcome::result<bool> verify(
        const BandersnatchSignature &signature,
        common::BufferView message,
        const BandersnatchPublicKey &public_key) const = 0;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, BandersnatchProviderError)
