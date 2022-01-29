/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SECP256K1_TYPES_HPP
#define KAGOME_CRYPTO_SECP256K1_TYPES_HPP

#include "common/blob.hpp"

namespace kagome::crypto::secp256k1 {
  namespace constants {
    static constexpr size_t kUncompressedPublicKeySize = 65u;
    static constexpr size_t kCompressedPublicKeySize = 33u;
    static constexpr size_t kCompactSignatureSize = 65u;
    static constexpr size_t kGeneralPublicKeySize = 64u;
  }  // namespace constants

  using Secp256k1VerifyError = uint8_t;
  namespace secp256k1_verify_error {
    static constexpr Secp256k1VerifyError kInvalidRS = 0;
    static constexpr Secp256k1VerifyError kInvalidV = 1;
    static constexpr Secp256k1VerifyError kInvalidSignature = 2;
  }  // namespace secp256k1_verify_error

  /**
   * compressed form of public key
   */
  using CompressedPublicKey = common::Blob<constants::kCompressedPublicKeySize>;

  /**
   * uncompressed form of public key
   */
  using UncompressedPublicKey =
      common::Blob<constants::kUncompressedPublicKeySize>;

  /**
   * truncated form of uncompressed public key
   */
  using PublicKey = common::Blob<constants::kGeneralPublicKeySize>;

  /**
   * secp256k1 RSV-signature
   */
  using RSVSignature = common::Blob<constants::kCompactSignatureSize>;

  /**
   * 32-byte sequence of bytes (presumably blake2s hash)
   */
  using MessageHash = common::Hash256;
}  // namespace kagome::crypto::secp256k1

#endif  // KAGOME_CRYPTO_SECP256K1_TYPES_HPP
