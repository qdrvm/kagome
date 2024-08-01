/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/bandersnatch_provider.hpp"
#include "crypto/bandersnatch_types.hpp"

namespace kagome::crypto {
  class Hasher;
}

namespace kagome::crypto {

  /**
   * sr25519 provider error codes
   */
  enum class BandersnatchProviderError {
    SIGN_UNKNOWN_ERROR = 1,  // unknown error occured during call to `sign`
                             // method of bound function
    VERIFY_UNKNOWN_ERROR,    // unknown error occured during call to `verify`
                             // method of bound function
    SOFT_JUNCTION_NOT_SUPPORTED,

  };

  class BandersnatchProviderImpl final : public BandersnatchProvider {
   public:
    BandersnatchProviderImpl(std::shared_ptr<Hasher> hasher);

    outcome::result<BandersnatchKeypair> generateKeypair(
        const BandersnatchSeed &seed, Junctions junctions) const override;

    outcome::result<BandersnatchSignature> sign(
        const BandersnatchKeypair &keypair,
        common::BufferView message) const override;

    outcome::result<bool> verify(
        const BandersnatchSignature &signature,
        common::BufferView message,
        const BandersnatchPublicKey &public_key) const override;

   private:
    std::shared_ptr<Hasher> hasher_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, BandersnatchProviderError)
