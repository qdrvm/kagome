/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/crypto_extension.hpp"

#include <exception>

#include "crypto/blake2/blake2s.h"
#include "crypto/twox/twox.hpp"

namespace kagome::extensions {
  void CryptoExtension::ext_blake2_256(const uint8_t *data, uint32_t len,
                                       uint8_t *out) {
    blake2s_256(out, data, len);
  }

  void CryptoExtension::ext_blake2_256_enumerated_trie_root(
      const uint8_t *values_data, const uint32_t *lens_data,
      uint32_t lens_length, uint8_t *result) {
    std::terminate();
  }

  uint32_t CryptoExtension::ext_ed25519_verify(const uint8_t *msg_data,
                                               uint32_t msg_len,
                                               const uint8_t *sig_data,
                                               const uint8_t *pubkey_data) {
    std::terminate();
  }

  void CryptoExtension::ext_twox_128(const uint8_t *data, uint32_t len,
                                     uint8_t *out) {
    out = kagome::crypto::make_twox128(data, len).data;
  }

  void CryptoExtension::ext_twox_256(const uint8_t *data, uint32_t len,
                                     uint8_t *out) {
    out = kagome::crypto::make_twox256(data, len).data;
  }
}  // namespace kagome::extensions
