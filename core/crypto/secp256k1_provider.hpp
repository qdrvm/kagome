/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP
#define KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP

#include <libp2p/crypto/secp256k1_provider.hpp>

#include "common/blob.hpp"
#include "outcome/outcome.hpp"

namespace kagome::crypto {
  namespace secp256k1 {
    static constexpr size_t kUncompressedPublicKeySize = 65u;
    static constexpr size_t kCompressedPublicKeySize = 33u;
    static constexpr size_t kCompactSignatureSize = 65u;
  }  // namespace secp256k1

  /**
   * uncompressed form of public key
   */
  using Secp256k1CompressedPublicKey =
      std::array<uint8_t, secp256k1::kCompressedPublicKeySize>;
  /**
   * compressed form of public key
   */
  using Secp256k1UncompressedPublicKey =
      std::array<uint8_t, secp256k1::kUncompressedPublicKeySize>;

  //  using Secp256k1PrivateKey = ::libp2p::crypto::secp256k1::PrivateKey;
  using Secp256k1Signature =
      std::array<uint8_t, secp256k1::kCompactSignatureSize>;

  /**
   * blake2s hash 32-byte sequence of bytes
   */
  using Secp256k1Message = common::Hash256;

  enum class Secp256k1ProviderError {
    INVALID_ARGUMENT = 1,
    RECOVERY_FAILED,
  };

  class Secp256k1Provider {
   public:
    virtual ~Secp256k1Provider() = 0;

    virtual outcome::result<Secp256k1UncompressedPublicKey>
    recoverPublickeyUncopressed(const Secp256k1Signature &signature,
                                const Secp256k1Message &message_hash) const = 0;

    virtual outcome::result<Secp256k1CompressedPublicKey>
    recoverPublickeyCompressed(const Secp256k1Signature &signature,
                               const Secp256k1Message &message_hash) const = 0;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, Secp256k1ProviderError);

#endif  // KAGOME_CRYPTO_SECP256K1_PROVIDER_HPP
