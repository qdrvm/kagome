/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_EXTENSION_HPP
#define KAGOME_CRYPTO_EXTENSION_HPP

#include <cstdint>

namespace kagome::extensions {
  /**
   * Implements extension functions related to cryptography
   */
  class CryptoExtension {
   public:
    /**
     * @see Extension::ext_blake2_256
     */
    void ext_blake2_256(const uint8_t *data, uint32_t len, uint8_t *out);

    /**
     * @see Extension::ext_blake2_256_enumerated_trie_root
     */
    void ext_blake2_256_enumerated_trie_root(const uint8_t *values_data,
                                             const uint32_t *lens_data,
                                             uint32_t lens_length,
                                             uint8_t *result);

    /**
     * @see Extension::ext_ed25519_verify
     */
    uint32_t ext_ed25519_verify(const uint8_t *msg_data, uint32_t msg_len,
                                const uint8_t *sig_data,
                                const uint8_t *pubkey_data);

    /**
     * @see Extension::ext_twox_128
     */
    void ext_twox_128(const uint8_t *data, uint32_t len, uint8_t *out);

    /**
     * @see Extension::ext_twox_256
     */
    void ext_twox_256(const uint8_t *data, uint32_t len, uint8_t *out);
  };
}  // namespace kagome::extensions

#endif  // KAGOME_CRYPTO_EXTENSION_HPP
