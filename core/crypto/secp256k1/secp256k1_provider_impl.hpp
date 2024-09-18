/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <secp256k1.h>

#include "crypto/secp256k1_provider.hpp"

namespace kagome::crypto {

  enum class Secp256k1ProviderError : uint8_t {
    INVALID_ARGUMENT = 1,
    INVALID_V_VALUE,
    INVALID_R_OR_S_VALUE,
    INVALID_SIGNATURE,
    RECOVERY_FAILED,
  };

  class Secp256k1ProviderImpl : public Secp256k1Provider {
   public:
    ~Secp256k1ProviderImpl() override = default;

    Secp256k1ProviderImpl();

    outcome::result<secp256k1::UncompressedPublicKey>
    recoverPublickeyUncompressed(const secp256k1::RSVSignature &signature,
                                 const secp256k1::MessageHash &message_hash,
                                 bool allow_overflow) const override;

    outcome::result<secp256k1::CompressedPublicKey> recoverPublickeyCompressed(
        const secp256k1::RSVSignature &signature,
        const secp256k1::MessageHash &message_hash,
        bool allow_overflow) const override;

   private:
    outcome::result<secp256k1_pubkey> recoverPublickey(
        const secp256k1::RSVSignature &signature,
        const secp256k1::MessageHash &message_hash,
        bool allow_overflow) const;

    std::unique_ptr<secp256k1_context, void (*)(secp256k1_context *)> context_;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Secp256k1ProviderError);
