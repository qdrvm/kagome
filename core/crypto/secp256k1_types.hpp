/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SECP256K1_TYPES_HPP
#define KAGOME_CRYPTO_SECP256K1_TYPES_HPP

#include "common/blob.hpp"

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

  using Secp256k1Signature =
      std::array<uint8_t, secp256k1::kCompactSignatureSize>;

  /**
   * blake2s hash 32-byte sequence of bytes
   */
  using Secp256k1Message = common::Hash256;

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SECP256K1_TYPES_HPP
